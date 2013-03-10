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

  /** Max of all the variables */
  double d_variableScoresMax;
  
  /** Values of variables */
  std::vector<bool> d_variableValues;
  
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
    
  /** How many restarts have happened */
  unsigned d_restartsCount;
  
  /** The base of the Luby sequence powers */
  double d_restartBase;

  /** The initial number of conflicts for the restart */
  unsigned d_restartInit;
  
  /** How many conflicts have happened */
  unsigned d_conflictsCount;
  
  /** How many conflicts until for a restart */
  unsigned d_conflictsLimit;

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
  
  /** Bump the heuristic value of the vaiable */
  void bumpVariable(Variable var);
  
  /** Bump the heuristic value of the clause */
  void bumpClause(CRef cRef);
  
  /** Nofification of a new conflict resolution step */
  void notifyConflictResolution(CRef clause);
  
  /** Notification of unset variables */
  void notifyVariableUnset(const std::vector<Variable>& vars);
    
  /** Notification of restarts */
  void notifyRestart();
  
  /** String representation */
  std::string toString() const { return "BCP Engine"; }

  /** Return the listener for new clauses */
  ClauseDatabase::INewClauseNotify* getNewClauseListener();
};

template class SolverPluginConstructor<BCPEngine>;

}
}
