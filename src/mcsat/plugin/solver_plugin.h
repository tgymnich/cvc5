#pragma once

#include "cvc4_private.h"

#include "expr/node.h"
#include "expr/kind.h"

#include "context/cdo.h"
#include "context/cdlist.h"

#include "mcsat/solver_trail.h"

namespace CVC4 {
namespace mcsat {

class Solver;

class SolverPluginRequest {

  /** The solver */
  Solver* d_solver;

public:

  /** Construct a request token for the given solver */
  SolverPluginRequest(Solver* solver)
  : d_solver(solver) {}

  /**
   * Ask for a backtrack to given level. The level must be smaller than the current decision level. The clause
   * must not be satisfied, and it should propagate at the given level.
   */
  void backtrack(unsigned level, CRef cRef);
};

/**
 * Base class for model based T-solvers.
 */
class SolverPlugin {

private:

  SolverPlugin() CVC4_UNUSED;
  SolverPlugin(const SolverPlugin&) CVC4_UNUSED;
  SolverPlugin& operator=(const SolverPlugin&) CVC4_UNUSED;

protected:

  /** The trail that the plugin should use */
  const SolverTrail& d_trail;

  /** Channel to request something from the solver */
  SolverPluginRequest& d_request;

  /** Construct the plugin. */
  SolverPlugin(const SolverTrail& trail, SolverPluginRequest& request)
  : d_trail(trail)
  , d_request(request)
  {}

  /** Get the SAT context associated to this Theory. */
  context::Context* getSearchContext() const {
    return d_trail.getSearchContext();
  }

public:

  /** Destructor */
  virtual ~SolverPlugin() {}

  /** Check the current assertions for consistency. */
  virtual void check() = 0;

  /** Perform propagation */
  virtual void propagate(SolverTrail::PropagationToken& out) = 0;

  /** Perform a decision */
  virtual void decide(SolverTrail::DecisionToken& out) = 0;

  /** Return the listener for new clauses */
  virtual ClauseDatabase::INewClauseNotify* getNewClauseListener() = 0;

}; /* class SolverPlugin */

} /* CVC4::mcsat namespace */
} /* CVC4 namespace */

#include "mcsat/plugin/solver_plugin_registry.h"
