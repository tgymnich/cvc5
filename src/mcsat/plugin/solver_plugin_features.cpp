#include "mcsat/plugin/solver_plugin_features.h"
#include "mcsat/plugin/solver_plugin.h"

using namespace CVC4;
using namespace mcsat;

void FeatureDispatch::addPlugin(SolverPlugin* plugin) {
  Debug("mcsat::plugin") << "FeatureDispatch::addPlugin(): " << *plugin << std::endl;
  const features_set& s = plugin->getFeaturesSet();
  features_set::const_iterator it = s.begin();
  for(; it != s.end(); ++ it) {
    PluginFeature type = *it;
    Debug("mcsat::plugin") << "FeatureDispatch::addPlugin(): " << type << std::endl;
    if (type >= d_plugins.size()) {
      d_plugins.resize(type + 1);
    }
    d_plugins[type].push_back(plugin);
  }
}

void FeatureDispatch::propagate(SolverTrail::PropagationToken& out) {
  for(unsigned i = 0; d_trail.consistent() && i < d_plugins[CAN_PROPAGATE].size(); ++ i) {
    SolverPlugin* plugin = d_plugins[CAN_PROPAGATE][i];
    Debug("mcsat::plugin") << "FeatureDispatch::propagate(): " << *plugin << std::endl;
    plugin->propagate(out);
  }
}

void FeatureDispatch::decide(SolverTrail::DecisionToken& out) {
  for(unsigned i = 0; !out.used() && i < d_plugins[CAN_DECIDE].size(); ++ i) {
    SolverPlugin* plugin = d_plugins[CAN_DECIDE][i];
    Debug("mcsat::plugin") << "FeatureDispatch::decide(): " << *plugin << std::endl;
    plugin->decide(out);
  }
}

