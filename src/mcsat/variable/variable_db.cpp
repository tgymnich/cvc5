#include "variable_db.h"

using namespace CVC4;
using namespace mcsat;

CVC4_THREADLOCAL(VariableDatabase*) VariableDatabase::s_current = 0;

VariableDatabase::Backtracker::Backtracker(context::Context* context, VariableDatabase& db)
: ContextNotifyObj(context)
, d_db(db)
{}

void VariableDatabase::Backtracker::contextNotifyPop() {
  for (unsigned toNotify = 0; toNotify < d_db.d_cd_notifySubscribers.size(); ++ toNotify) {
    INewVariableNotify* notify = d_db.d_cd_notifySubscribers[toNotify];
    for (unsigned i = d_db.d_variablesCountAtCurrentLevel; i < d_db.d_variableNodes.size(); ++ i) {
      notify->newVariable(d_db.d_variables[i]);
    }
  }
  d_db.d_variablesCountAtCurrentLevel = d_db.d_variables.size();
}

VariableDatabase::VariableDatabase(context::Context* context)
: d_context(context)
, d_variablesCountAtCurrentLevel(context, 0)
, d_backtracker(context, *this)
{
  Assert(s_current == 0, "Make sure active DB is 0 before creating a new VariableDatabase");
  s_current = this;
  Debug("mcsat::var_db") << "VariableDatabase(): var_null = " << +Variable::s_null_varId << std::endl;
}

size_t VariableDatabase::getTypeIndex(TypeNode type) {
  typenode_to_variable_map::const_iterator find_type = d_typenodeToVariableMap.find(type);
  size_t typeIndex;
  if (find_type == d_typenodeToVariableMap.end()) {
    typeIndex = d_variableTypes.size();
    Debug("mcsat::var_db") << "VariableDatabase::getTypeIndex(" << type << ") => " << typeIndex << std::endl;
    d_typenodeToVariableMap[type] = typeIndex;
    d_variableTypes.push_back(type);
    d_variableNodes.resize(typeIndex + 1);
    d_variableRefCount.resize(typeIndex + 1);
  } else {
    typeIndex = find_type->second;
  }
  return typeIndex;
}

Variable VariableDatabase::getVariable(TNode node) {

  // Find the variable (it might exist already)
  node_to_variable_map::const_iterator find = d_nodeToVariableMap.find(node);

  // If the variable is there, we just return it
  if (find != d_nodeToVariableMap.end()) {
    // No need to notify, people were notified already
    return find->second;
  }

  // The type of the variable
  TypeNode type = node.getType();
  size_t typeIndex = getTypeIndex(type);

  // The id of the new variable
  size_t newVarId = d_variableNodes[typeIndex].size();

  // Add the information
  d_variableNodes[typeIndex].push_back(node);
  d_variableRefCount[typeIndex].push_back(0);

  Variable var(newVarId, typeIndex);

  d_nodeToVariableMap[node] = var;
  d_variables.push_back(var);

  Debug("mcsat::var_db") << "VariableDatabase::newVariable(" << node << ") => " << var.index() << std::endl;

  for (unsigned toNotify = 0; toNotify < d_notifySubscribers.size(); ++ toNotify) {
    d_notifySubscribers[toNotify]->newVariable(var);
  }
  d_variablesCountAtCurrentLevel = d_variables.size();

  return var;
}

void VariableDatabase::addNewVariableListener(INewVariableNotify* listener) {
  Assert(d_context->getLevel() == 0);
  if (listener->isContextDependent()) {
    d_cd_notifySubscribers.push_back(listener);
  }
  d_notifySubscribers.push_back(listener);
}
