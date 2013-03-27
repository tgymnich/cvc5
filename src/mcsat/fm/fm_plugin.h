#pragma once

#include "mcsat/plugin/solver_plugin.h"

namespace CVC4 {
namespace mcsat {

/**
 * Fourier-Motzkin elimination plugin.
 */
class FMPlugin : public SolverPlugin {

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




