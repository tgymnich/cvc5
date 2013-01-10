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

  /** Delayed propagations (c[0] is propagated) */
  std::vector<CRef> d_delayedPropagations;

  /** New clause */
  void newClause(CRef cref);

  /** Head pointer into the trail */
  context::CDO<size_t> d_trailHead;

public:
  
  /** New propagation engine */
  BCPEngine(const SolverTrail& trail, SolverPluginRequest& request);
  
  /** BCP does nothing in checks */
  void check() {}

  /** Perform a propagation */
  void propagate(SolverTrail::PropagationToken& out);

  /** Perform a decision */
  void decide(SolverTrail::DecisionToken& out);

  /** Return the listener for new clauses */
  ClauseDatabase::INewClauseNotify* getNewClauseListener();
};

template class SolverPluginConstructor<BCPEngine>;

}
}
