#include "mcsat/solver.h"

using namespace CVC4;
using namespace CVC4::mcsat;

void SolverPluginRequest::backtrack(unsigned level, CRef cRef) {
  d_solver->requestBacktrack(level, cRef);
}
