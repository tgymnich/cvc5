#pragma once

#include "mcsat/rules/proof_rule.h"
#include "mcsat/fm/linear_constraint.h"
#include "mcsat/solver_trail.h"

namespace CVC4 {
namespace mcsat {
namespace rules {

/**
 * Boolean resolution rule. To be used in sequence for one resolution proof.
 */
class FourierMotzkinRule : public ProofRule {

  /** The set of assumptions */
  std::set<Literal> d_assumptions;

  /** Resolvent */
  fm::LinearConstraint d_resolvent;

  /** The trail */
  const SolverTrail& d_trail;

public:

  /** Create a new Fourier-Motzkin resolution */
  FourierMotzkinRule(ClauseDatabase& clauseDB, const SolverTrail& trail);

  /** Start the resolution */
  void start(Literal ineq);

  /** Resolve with given inequality over the given variable. */
  void resolve(Variable var, Literal ineq);

  /**
   * Finish the derivation. We need the prop token, since the newly created literal will evaluate at lower level.
   */
  CRef finish(SolverTrail::PropagationToken& propToken);
  
};

}
}
}



