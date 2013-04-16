#include "mcsat/clause/clause_db.h"

#include <cstdlib>
#include <iostream>

using namespace std;

using namespace CVC4;
using namespace CVC4::mcsat;

static const size_t d_initialMemorySize = 100000;

CVC4_THREADLOCAL(ClauseFarm*) ClauseFarm::s_current = 0;

ClauseDatabase::Backtracker::Backtracker(context::Context* context, ClauseDatabase& db)
: ContextNotifyObj(context)
, d_db(db)
{}

void ClauseDatabase::Backtracker::contextNotifyPop() {
  for (unsigned toNotify = 0; toNotify < d_db.d_cd_notifySubscribers.size(); ++ toNotify) {
    INewClauseNotify* notify = d_db.d_cd_notifySubscribers[toNotify];
    for (unsigned i = d_db.d_firstNotNotified; i < d_db.d_clausesList.size(); ++ i) {
      notify->newClause(d_db.d_clausesList[i]);
    }
  }
  d_db.d_firstNotNotified = d_db.d_clausesList.size();
}


ClauseDatabase::ClauseDatabase(context::Context* context, std::string name, size_t id)
: d_memory(0)
, d_size(0)
, d_capacity(0)
, d_rules(0)
, d_id(id)
, d_name(name)
, d_backtracker(context, *this)
, d_context(context)
, d_firstNotNotified(context, 0)
{
  d_memory = (char*)std::malloc(d_initialMemorySize);
  d_capacity = d_initialMemorySize;
}

ClauseDatabase::~ClauseDatabase() {
  // Delete all the clauses
  for (unsigned i = 0; i < d_clausesList.size(); ++ i) {
    getClause(d_clausesList[i]).~Clause();
  }
  free(d_memory);
}

char* ClauseDatabase::allocate(size_t& size, char*& mem, size_t& memSize, size_t& memCapacity) {
  // The memory we are dealing with
  // Align the size
  size = align(size);
  // Ensure enough capacity
  size_t requested = memSize + size;
  if (requested > memCapacity) {
    while (requested > memCapacity) {
      memCapacity += memCapacity >> 1;
    }
    // Reallocate the memory
    mem = (char*)std::realloc(mem, memCapacity);
    if (mem == NULL) {
      std::cerr << "Out of memory" << std::endl;
      exit(1);
    }
  }
  // The pointer to the new memory
  char* pointer = mem + memSize;
  // Increase the used size
  memSize += size;
  // And that's it
  return pointer;
}

CRef ClauseDatabase::newClause(const LiteralVector& literals, size_t ruleId) {

  Debug("mcsat::clause_db") << "ClauseDatabase::newClause(" << literals << ", " << ruleId << ")" << std::endl;

  // Compute the size (this should be safe as variant puts the template data at the end)
  size_t size = sizeof(Clause) + sizeof(Literal)*literals.size();

  // Allocate the memory
  char* memory = allocate(size, d_memory, d_size, d_capacity);

  // Construct the clause
  new (memory) Clause(literals, ruleId);

  // The new reference (not reference counted for now)
  CRef cRef(memory - d_memory, d_id);

  Debug("mcsat::clause_db") << "ClauseDatabase::newClause() => " << cRef.d_ref << std::endl;

  // Notify
  for (unsigned toNotify = 0; toNotify < d_notifySubscribers.size(); ++ toNotify) {
    d_notifySubscribers[toNotify]->newClause(cRef);
  }
  d_clausesList.push_back(cRef);
  d_firstNotNotified = d_clausesList.size();

  // The new clause
  return cRef;
}

void ClauseDatabase::addNewClauseListener(INewClauseNotify* listener) const {
  Assert(d_context->getLevel() == 0);
  ClauseDatabase* nonconst = const_cast<ClauseDatabase*>(this);
  if (listener->isContextDependent()) {
    nonconst->d_cd_notifySubscribers.push_back(listener);
  }
  nonconst->d_notifySubscribers.push_back(listener);
}

ClauseFarm::ClauseFarm(context::Context* context)
: d_context(context)
{
  Assert(s_current == 0, "Don't create clause farms with an active farm");
  s_current = this;
}

ClauseFarm::~ClauseFarm() {
  for (unsigned i = 0; i < d_databases.size(); ++ i) {
    delete d_databases[i];
  }  
  if (s_current == this) {
    s_current = 0;
  }
}

CRef ClauseDatabase::adopt(CRef toAdopt) {

  Assert(toAdopt.getDatabaseId() != d_id, "No reason to adopt my own clauses");

  // The clause to adopt
  Clause& clause = toAdopt.getClause();

  // Compute the size (this should be safe as variant puts the template data at the end)
  size_t size = sizeof(Clause) + sizeof(Literal)*clause.size();

  // Allocate the memory
  char* memory = allocate(size, d_memory, d_size, d_capacity);

  // Construct the clause
  new (memory) Clause(clause);

  // The new reference (not reference counted for now)
  CRef cRef(memory - d_memory, d_id);

  // Notify
  for (unsigned toNotify = 0; toNotify < d_notifySubscribers.size(); ++ toNotify) {
    d_notifySubscribers[toNotify]->newClause(cRef);
  }
  d_clausesList.push_back(cRef);
  d_firstNotNotified = d_clausesList.size();

  // The new clause
  return cRef;
}

void ClauseDatabase::performGC(const std::set<CRef>& clausesToKeep, ClauseRelocationInfo& clauseRelocationInfo) {

  char* memoryNew = (char*)std::malloc(d_capacity);
  size_t capacityNew = d_capacity;
  size_t sizeNew = 0;

  std::vector<CRef> clausesListNew;

  for (unsigned i = 0; i < d_clausesList.size(); ++ i) {
    CRef oldClauseRef = d_clausesList[i];
    if (clausesToKeep.count(oldClauseRef) == 0) {

      Debug("mcsat::gc") << "GC: collecting " << oldClauseRef << std::endl;

      // Old clause
      Clause& clause = oldClauseRef.getClause();
      // Size of the clause
      size_t size = sizeof(Clause) + sizeof(Literal)*clause.size();
      // Where to put the new clause
      char* memory = allocate(size, memoryNew, sizeNew, capacityNew);
      // Copy the content
      memcpy(memoryNew, &clause, size);
      // New reference
      CRef newClauseRef(memory - memoryNew, d_id);
      clauseRelocationInfo.add(oldClauseRef, newClauseRef);
      // Add to the list
      clausesListNew.push_back(newClauseRef);
    }
  }

  // Free the old memory
  std::free(d_memory);

  d_memory = memoryNew;
  d_size = sizeNew;
  d_capacity = capacityNew;
  d_clausesList.swap(clausesListNew);

  d_firstNotNotified = d_clausesList.size();
}

void ClauseRelocationInfo::add(CRef oldClause, CRef newClause) {
  Assert(d_map.find(oldClause) == d_map.end());
  d_map[oldClause] = newClause;
}

CRef ClauseRelocationInfo::relocate(CRef oldClause) const {
  relocation_map::const_iterator find = d_map.find(oldClause);
  if (find == d_map.end()) {
    return CRef::null;
  } else {
    return find->second;
  }
}

void ClauseRelocationInfo::relocate(std::vector<CRef>& clauses) const {
  unsigned current = 0;
  unsigned lastToKeep = 0;
  for (; current < clauses.size(); ++ current) {
    CRef newClause = relocate(clauses[current]);
    if (!newClause.isNull()) {
      clauses[lastToKeep ++] = newClause;
    }
  }
  clauses.resize(lastToKeep);
}
