#include "mcsat/plugin/solver_plugin_features.h"
#include "mcsat/plugin/solver_plugin.h"

using namespace CVC4;
using namespace mcsat;

using namespace std;

void FeatureDispatch::addPlugin(SolverPlugin* plugin) {
  const features_set& s = plugin->getFeaturesSet();
  features_set::const_iterator it = s.begin();
  for(; it != s.end(); ++ it) {
    PluginFeature type = *it;
    if (type >= d_plugins.size()) {
      d_plugins.resize(type + 1);
    }
    d_plugins[type].push_back(plugin);
  }
}

void FeatureDispatch::propagate(SolverTrail::PropagationToken& out) {
  for(unsigned i = 0; d_trail.consistent() && i < d_plugins[CAN_PROPAGATE].size(); ++ i) {
    d_plugins[CAN_PROPAGATE][i]->propagate(out);
  }
}

void FeatureDispatch::decide(SolverTrail::DecisionToken& out) {
  for(unsigned i = 0; i < !out.used() && d_plugins[CAN_DECIDE].size(); ++ i) {
    d_plugins[CAN_DECIDE][i]->decide(out);
  }
}

