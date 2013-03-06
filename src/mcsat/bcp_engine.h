#pragma once

#include "mcsat/plugin/solver_plugin.h"
#include "mcsat/watch_list_manager.h"

#include <ext/pb_ds/priority_queue.hpp>

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

  class NewVariableNotify : public VariableDatabase::INewVariableNotify {
    BCPEngine& d_engine;
    size_t d_bool_type_index;
  public:
    NewVariableNotify(BCPEngine& engine);
    void newVariable(Variable var);
  };

  /** Notification for new clauses */
  NewClauseNotify d_newClauseNotify;

  /** Notification for new variables */
  NewVariableNotify d_newVariableNotify;

  /** Delayed propagations (c[0] is propagated) */
  std::vector<CRef> d_delayedPropagations;

  /** New clause */
  void newClause(CRef cref);

  /** New variable */
  void newVariable(Variable var);

  /** Head pointer into the trail */
  context::CDO<size_t> d_trailHead;

  /** Scores of variables */
  std::vector<double> d_variableScores;

  /** Compare variables according to their current score */
  class VariableScoreCmp {
    std::vector<double>& d_variableScores;
  public:
    VariableScoreCmp(std::vector<double>& variableScores)
    : d_variableScores(variableScores) {}
    bool operator() (const Variable& v1, const Variable& v2) const {
      return d_variableScores[v1.index()] < d_variableScores[v2.index()];
    }
  } d_variableScoreCmp;

  /** Priority queue type we'll be using to keep the variables to pick */
  typedef __gnu_pbds::priority_queue<Variable, VariableScoreCmp> variable_queue;

  /** Priority queue for the variables */
  variable_queue d_variableQueue;

  /** Position in the variable queue */
  std::vector<variable_queue::point_iterator> d_variableQueuePositions;

  /** Enqueues the variable for decision making */
  void enqueue(Variable var);

  /** Is the variable in queue */
  bool inQueue(Variable var) const;
  
public:
  
  /** New propagation engine */
  BCPEngine(const SolverTrail& trail, SolverPluginRequest& request);
  
  /** BCP does nothing in checks */
  void check() {}

  /** Perform a propagation */
  void propagate(SolverTrail::PropagationToken& out);

  /** Perform a decision */
  void decide(SolverTrail::DecisionToken& out);

  /** Notification of a new conflict */
  void notifyConflict();
  
  /** Nofification of a new conflict resolution step */
  void notifyConflictResolution(CRef clause);
  
  /** Notification of unset variables */
  void notifyVariableUnset(const std::vector<Variable>& vars);
    
  /** String representation */
  std::string toString() const { return "BCP Engine"; }

  /** Return the listener for new clauses */
  ClauseDatabase::INewClauseNotify* getNewClauseListener();
};

template class SolverPluginConstructor<BCPEngine>;

}
}
