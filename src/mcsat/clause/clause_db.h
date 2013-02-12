#pragma once

#include "cvc4_private.h"

#include <vector>
#include <iostream>
#include <hash_map>

#include "util/tls.h"
#include "context/context.h"
#include "context/cdo.h"

#include "mcsat/clause/clause.h"
#include "mcsat/clause/clause_ref.h"

namespace CVC4 {
namespace mcsat {

namespace rules {
class ProofRule;
}

/** A database of clauses */
class ClauseDatabase {

public:

  /** Notification interface for creation of new clauses */
  class INewClauseNotify {
    /** Is this listener context dependent */
    bool d_isContextDependent;

  public:

    INewClauseNotify(bool isCD)
    : d_isContextDependent(isCD) {}

    /** Is this listener context dependent */
    bool isContextDependent() const {
      return d_isContextDependent;
    }

    virtual ~INewClauseNotify() {}
    virtual void newClause(CRef cref) = 0;
  };

  /** Relocation map for references */
  typedef __gnu_cxx::hash_map<CRef, CRef> RellocationMap;

  /** Notification interface for garbage collection */
  class IGCNotify {
  public:
    virtual ~IGCNotify() {}
    virtual void notify(const RellocationMap& relocationMap) = 0;
  };

private:
  
  /** Pointer to the memory */
  char* d_memory;

  /** Use of the current allocated memory */
  unsigned d_size;

  /** Capacity of the current allocated memory */
  unsigned d_capacity;

  /** Wasted memory */
  unsigned d_wasted;

  inline unsigned align(unsigned size) {
    return (size + 7) & ~((unsigned)7);
  }

  /** Allocate new memory */
  char* allocate(unsigned size) __attribute__ ((malloc));

  template<bool refCount>
  friend class ClauseRef;

  /**
   * Get the clause given the reference
   */
  Clause& getClause(ClauseRef<false> cRef) {
    return *((Clause*) (d_memory + cRef.d_ref));
  }

  friend class rules::ProofRule;

  /** Number of rules attached to this clause database */
  size_t d_rules;
  
  /** Register a new rule */
  size_t registerRule() {
    return d_rules ++;
  }
  
  /**
   * Add a clause to the database (only proof rules can do this).
   */
  CRef newClause(const LiteralVector& literals, size_t ruleId);

  friend class ClauseFarm;
  
  /** The id of the database */
  size_t d_id; 
  
  /** Name of the clause database */
  std::string d_name;
  
  /** Construct a clause database */
  ClauseDatabase(context::Context* context, std::string name, size_t id);

  /** Pop notifications go through this class */
  class Backtracker : public context::ContextNotifyObj {
    ClauseDatabase& d_db;
  public:
    Backtracker(context::Context* context, ClauseDatabase& db);
    void contextNotifyPop();
  };

  /** Backtracker for notifications context pops */
  Backtracker d_backtracker;

  /** Backtracker is a friend to be able to notify listeners on backtrack */
  friend class Backtracker;

  /** The context we're using */
  context::Context* d_context;

  /** Last notified clause */
  context::CDO<size_t> d_firstNotNotified;

  /** Non-context-dependent notify subscribers */
  std::vector<INewClauseNotify*> d_notifySubscribers;

  /** Context dependent notify subscribers */
  std::vector<INewClauseNotify*> d_cd_notifySubscribers;

  /** The list of all clauses in this db */
  std::vector<CRef> d_clausesList;

public:

  ~ClauseDatabase();

  /** Import a clause from a different db (in the same farm) */
  CRef adopt(CRef cRef);
  
  /**
   * Get the clause given the reference
   */
  const Clause& getClause(ClauseRef<false> cRef) const {
    return *((const Clause*) (d_memory + cRef.d_ref));
  }

  /**
   * Add a listener for the creation of new clauses. A context independent listener will only be notified
   * once when the clause is created. If the listener is context dependent, it will be notified when the clause is
   * created, but it will be re-notified on every pop so that it can update it's internal state.
   *
   * @param listener the listener
   */
  void addNewClauseListener(INewClauseNotify* listener) const;

  /**
   * Add a listener for the garbage collection (you are managing it). Garbage collection only happens at 0 context
   * level.
   */
  void addGCListener(IGCNotify* listener);

private:

  /** List of all GC listeners */
  std::vector<IGCNotify*> d_gcListeners;
};

class ClauseFarm {

  /** Default clause farm to use */
  static CVC4_THREADLOCAL(ClauseFarm*) s_current;
  
  /** The databases of this farm */
  std::vector<ClauseDatabase*> d_databases;
  
  /** Context that the database obeys */
  context::Context* d_context;

public:

  /** Construct a clause farm and set it as current */
  ClauseFarm(context::Context* context);

  ~ClauseFarm();
  
  static ClauseFarm* getCurrent() { 
    return s_current;
  }

  static void setCurrent(ClauseFarm* current) { 
    s_current = current;
  }

  /** Create a new clause database with this farm */
  ClauseDatabase& newClauseDB(std::string name) {
    d_databases.push_back(new ClauseDatabase(d_context, name, d_databases.size()));
    return *d_databases.back();
  }

  /** Get the database with the given id */
  ClauseDatabase& getClauseDB(size_t id) {
    return *d_databases[id];
  }
  
  /**
   * Helper class to ensure scoped clause farm.
   */
  class SetCurrent {
    ClauseFarm* old;
  public:
    /** Set the given DB in the current scope. */
    SetCurrent(ClauseFarm* db) {
      old = getCurrent();
      setCurrent(db);
    }
    ~SetCurrent() {
      setCurrent(old);
    }
  };
};


} /* namespace mcsat */

} /* namespace CVC4 */
