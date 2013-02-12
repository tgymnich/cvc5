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
, d_wasted(0)
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

char* ClauseDatabase::allocate(unsigned size) {
  // The memory we are dealing with
  // Align the size
  size = align(size);
  // Ensure enough capacity
  size_t requested = d_size + size;
  if (requested > d_capacity) {
    while (requested > d_capacity) {
      d_capacity += d_capacity >> 1;
    }
    // Reallocate the memory
    d_memory = (char*)std::realloc(d_memory, d_capacity);
    if (d_memory == NULL) {
      std::cerr << "Out of memory" << std::endl;
      exit(1);
    }
  }
  // The pointer to the new memory
  char* pointer = d_memory + d_size;
  // Increase the used size
  d_size += size;
  // And that's it
  return pointer;
}

CRef ClauseDatabase::newClause(const LiteralVector& literals, size_t ruleId) {

  Debug("mcsat::clause_db") << "ClauseDatabase::newClause(" << literals << ", " << ruleId << ")" << std::endl;

  // Compute the size (this should be safe as variant puts the template data at the end)
  size_t size = sizeof(Clause) + sizeof(Clause::literal_type)*literals.size();

  // Allocate the memory
  char* memory = allocate(size);

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

CRef ClauseDatabase::adopt(CRef toAdopt) {
  
  Assert(toAdopt.getDatabaseId() != d_id, "No reason to adopt my own clauses");
  
  // The clause to adopt
  Clause& clause = toAdopt.getClause(); 
  
  // Compute the size (this should be safe as variant puts the template data at the end)
  size_t size = sizeof(Clause) + sizeof(int)*clause.size();

  // Allocate the memory
  char* memory = allocate(size);

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

void ClauseDatabase::addGCListener(IGCNotify* listener) {
  d_gcListeners.push_back(listener);
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
}
