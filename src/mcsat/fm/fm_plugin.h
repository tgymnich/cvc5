#pragma once

#include "mcsat/plugin/solver_plugin.h"

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
  size_t d_int_type_index;

  /** Real type index */
  size_t d_real_type_index;

  /** Called on new real variables */
  void newVariable(Variable var);

  /** Called on arithmetic constraints */
  void newConstraint(Variable constraint);
  
public:

  /** Constructor */
  FMPlugin(ClauseDatabase& clauseDb, const SolverTrail& trail, SolverPluginRequest& request);

  /** Perform propagation */
  void propagate(SolverTrail::PropagationToken& out);

  /** Perform a decision */
  void decide(SolverTrail::DecisionToken& out);

  /** String representation of the plugin (for debug purposes mainly) */
  std::string toString() const;
};

template class SolverPluginConstructor<FMPlugin>;

}
}




