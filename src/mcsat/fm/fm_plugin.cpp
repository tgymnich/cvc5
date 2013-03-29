#include "mcsat/fm/fm_plugin.h"

using namespace CVC4;
using namespace mcsat;

FMPlugin::NewVariableNotify::NewVariableNotify(FMPlugin& plugin)
: INewVariableNotify(false)
, d_plugin(plugin)
{
}

void FMPlugin::NewVariableNotify::newVariable(Variable var) {
  // Register new variables
  if (var.typeIndex() == d_plugin.d_real_type_index || var.typeIndex() == d_plugin.d_int_type_index) {
    d_plugin.newVariable(var);
  } else {
    // Register arithmetic constraints
    TNode varNode = var.getNode();
    switch (varNode.getKind()) {
      case kind::LT: 
      case kind::LEQ:
      case kind::GT:
      case kind::GEQ:
	// Arithmetic constraints
	d_plugin.newConstraint(var);
	break;
      case kind::EQUAL:
	// TODO: Only if both sides are arithmetic
	d_plugin.newConstraint(var);
	break;
      default:
	// Ignore
	break;
    }
  }
}

FMPlugin::FMPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request)
: SolverPlugin(database, trail, request)
, d_newVariableNotify(*this)
{
  Debug("mcsat::fm") << "FMPlugin::FMPlugin()" << std::endl;

  // We decide values of variables and can detect conflicts
  addFeature(CAN_PROPAGATE);
  addFeature(CAN_DECIDE);

  // Types we care about
  VariableDatabase& db = *VariableDatabase::getCurrentDB();
  d_int_type_index = db.getTypeIndex(NodeManager::currentNM()->integerType());
  d_real_type_index = db.getTypeIndex(NodeManager::currentNM()->realType());

  // Add the listener
  db.addNewVariableListener(&d_newVariableNotify);
}

std::string FMPlugin::toString() const  {
  return "Fourier-Motzkin Elimination (Model-Based)";
}

void FMPlugin::newVariable(Variable var) {
  Debug("mcsat::fm") << "FMPlugin::newVariable(" << var << ")" << std::endl;
}

void FMPlugin::newConstraint(Variable constraint) {
  Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << ")" << std::endl;
}

void FMPlugin::propagate(SolverTrail::PropagationToken& out) {
  Debug("mcsat::fm") << "FMPlugin::propagate()" << std::endl;
}

void FMPlugin::decide(SolverTrail::DecisionToken& out) {
  Debug("mcsat::fm") << "FMPlugin::decide()" << std::endl;
}
