#include "mcsat/fm/fm_plugin.h"

using namespace CVC4;
using namespace mcsat;

FMPlugin::FMPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request)
: SolverPlugin(database, trail, request)
{
  // We decide values of variables and can detect conflicts
  addFeature(CAN_PROPAGATE);
  addFeature(CAN_DECIDE);
}

std::string FMPlugin::toString() const  {
  return "Fourier-Motzkin Elimination (Model-Based)";
}

void FMPlugin::propagate(SolverTrail::PropagationToken& out) {

}

void FMPlugin::decide(SolverTrail::DecisionToken& out) {

}
