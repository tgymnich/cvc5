#pragma once

#include "cvc4_private.h"

#include "expr/node.h"
#include "expr/kind.h"

#include "context/cdo.h"
#include "context/cdlist.h"

#include "mcsat/solver_trail.h"
#include "mcsat/plugin/solver_plugin_registry.h"

namespace CVC4 {
namespace mcsat {

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

  /** Construct the plugin. */
  SolverPlugin(const SolverTrail& trail)
  : d_trail(trail)
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

  /** Return the listener for new clauses */
  virtual ClauseDatabase::INewClauseNotify* getNewClauseListener() = 0;

}; /* class SolverPlugin */

} /* CVC4::mcsat namespace */
} /* CVC4 namespace */
