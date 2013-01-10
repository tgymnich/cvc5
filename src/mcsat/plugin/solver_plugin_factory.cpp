#include "mcsat/plugin/solver_plugin_factory.h"
#include "mcsat/plugin/solver_plugin_registry.h"

#include <sstream>

using namespace CVC4;
using namespace CVC4::mcsat;

SolverPlugin* SolverPluginFactory::create(std::string name, const SolverTrail& trail, SolverPluginRequest& request)
  throw(SolverPluginFactoryException)
{
  ISolverPluginConstructor* constructor = SolverPluginRegistry::getConstructor(name);
  if (constructor) {
    return constructor->construct(trail, request);
  } else {

    std::stringstream ss;
    ss << "Solver " << name << " not available. Available plugins are:";

    std::vector<std::string> plugins;
    SolverPluginRegistry::getAvailablePlugins(plugins);
    std::ostream_iterator<std::string> out_it (ss, ", ");
    std::copy(plugins.begin(), plugins.end(), out_it);

    throw SolverPluginFactoryException(ss.str());
  }
}

