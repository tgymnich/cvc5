#pragma once

#include "mcsat/plugin/solver_plugin.h"
#include "mcsat/variable/variable_table.h"

#include "mcsat/fm/fm_plugin_types.h"

#include "mcsat/util/assigned_watch_manager.h"
#include "mcsat/util/var_priority_queue.h"

#include "mcsat/rules/fourier_motzkin_rule.h"

namespace CVC4 {
namespace mcsat {

/**
 * Fourier-Motzkin elimination plugin.
 */
class FMPlugin : public SolverPlugin {

  class NewVariableNotify : public VariableDatabase::INewVariableNotify {
    FMPlugin& d_plugin;
  public:
    NewVariableNotify(FMPlugin& d_plugin);
    void newVariable(Variable var);
  } d_newVariableNotify;

  /** Integer type index */
  size_t d_intTypeIndex;

  /** Real type index */
  size_t d_realTypeIndex;

  /** Delayed constraints to propagate by value (value is propagated) */
  std::vector<Variable> d_delayedPropagations;

  /** Called on new real variables */
  void newVariable(Variable var);

  /** Called on arithmetic constraints */
  void newConstraint(Variable constraint);
  
  /** Number of unassigned variables in a constraint */
  enum UnassignedStatus {
    UNASSIGNED_UNKNOWN,
    UNASSIGNED_UNIT,
    UNASSIGNED_NONE,
  };

  /** Map from constraints to their status */
  std::vector<UnassignedStatus> d_constraintUnassignedStatus;

  /** List of fixed variables to use for decisions x = c assertion */
  context::CDList<Variable> d_fixedVariables;

  /** Index of the last fixed variable */
  context::CDO<unsigned> d_fixedVariablesIndex;

  /** Number of fixed variable decisions */
  context::CDO<unsigned> d_fixedVariablesDecided;

  /** Map from variables to constraints */
  fm::var_to_constraint_map d_constraints;

  /** Returns true if variable is a registered linear arith constraint */
  bool isLinearConstraint(Variable var) const {
    return d_constraints.find(var) != d_constraints.end();
  }

  const fm::LinearConstraint& getLinearConstraint(Variable var) const {
    Assert(isLinearConstraint(var));
    return d_constraints.find(var)->second;
  }

  /** Is this variable an arithmetic variable */
  bool isArithmeticVariable(Variable var) const {
    return var.typeIndex() == d_realTypeIndex || var.typeIndex() == d_intTypeIndex;
  }

  /** Head pointer into the trail */
  context::CDO<size_t> d_trailHead;

  /** Bounds in the current context */
  fm::CDBoundsModel d_bounds;
  
  /** Watch manager for assigned variables */
  util::AssignedWatchManager d_assignedWatchManager;

  /** Checks whether the constraint is unit */
  bool isUnitConstraint(Variable constraint);

  /**
   * Takes a unit constraint and asserts the apropriate bound (if inequalities), or
   * adds to the list of dis-equalities for the free variable.
   */
  void processUnitConstraint(Variable constraint);

  /** The Fourier-Motzkin rule we use for derivation */
  rules::FourierMotzkinRule d_fmRule;

  /** Priority queue for variable selection */
  util::VariablePriorityQueue d_variableQueue;

  /**
   * Processes any conflicts.
   */
  void processConflicts(SolverTrail::PropagationToken& out);

public:

  /** Constructor */
  FMPlugin(ClauseDatabase& clauseDb, const SolverTrail& trail, SolverPluginRequest& request);

  /** Perform propagation */
  void propagate(SolverTrail::PropagationToken& out);

  /** Perform a decision */
  void decide(SolverTrail::DecisionToken& out);

  /** String representation of the plugin (for debug purposes mainly) */
  std::string toString() const;

  /** Notification of unset variables */
  void notifyBackjump(const std::vector<Variable>& vars);

  /** Mark phase of the GC */
  void gcMark(std::set<Variable>& varsToKeep, std::set<CRef>& clausesToKeep);

  /** Relocation phase of the GC */
  void gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc);

};

// Register the plugin
MCSAT_REGISTER_PLUGIN(FMPlugin);

}
}



