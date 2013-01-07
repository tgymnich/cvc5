#pragma once

#include "mcsat/plugin/solver_plugin.h"

#include "mcsat/watch_list_manager.h"

namespace CVC4 {
namespace mcsat {
  
/**
 * Boolean constraint propagation (BCP) engine.
 */
class BCPEngine : public SolverPlugin {
  
  /** Watch lists */
  WatchListManager d_watchManager;

  class NewClauseNotify : public ClauseDatabase::INewClauseNotify {
    BCPEngine& d_engine;
  public:
    NewClauseNotify(BCPEngine& d_engine);
    void newClause(CRef cref);
  };

  /** Notification for new clauses */
  NewClauseNotify d_newClauseNotify;

  /** Added unit clause */
  std::vector<CRef> d_unitClauses;

  /** New clause */
  void newClause(CRef cref);

public:
  
  /** New propagation engine */
  BCPEngine(const SolverTrail& trail);
  
  /** BCP does nothing in checks */
  void check() {}

  /** Perform propagation */
  void propagate(SolverTrail::PropagationToken& out);

  /** Return the listener for new clauses */
  ClauseDatabase::INewClauseNotify* getNewClauseListener();
};

template class SolverPluginConstructor<BCPEngine>;

}
}
