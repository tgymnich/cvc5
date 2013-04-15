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
  typenode_to_id_map::const_iterator find_type = d_typenodeToIdMap.find(type);
  size_t typeIndex;
  if (find_type == d_typenodeToIdMap.end()) {
    typeIndex = d_variableTypes.size();
    Debug("mcsat::var_db") << "VariableDatabase::getTypeIndex(" << type << ") => " << typeIndex << std::endl;
    d_typenodeToIdMap[type] = typeIndex;
    d_variableTypes.push_back(type);
    d_variableNodes.resize(typeIndex + 1);
  } else {
    typeIndex = find_type->second;
  }
  return typeIndex;
}

size_t VariableDatabase::size(size_t typeIndex) const {
  return d_variableNodes[typeIndex].size();
}

bool VariableDatabase::hasVariable(TNode node) const {
  return d_nodeToVariableMap.find(node) != d_nodeToVariableMap.end();
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

  Variable var(newVarId, typeIndex);

  // Add to the node map
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

  for (unsigned typeIndex = 0; typeIndex < d_variableTypes.size(); ++ typeIndex) {
    for (unsigned variableIndex = 0; variableIndex < d_variableNodes[typeIndex].size(); ++ variableIndex) {
      listener->newVariable(Variable(variableIndex, typeIndex));
    }
  }
  
  if (listener->isContextDependent()) {
    d_cd_notifySubscribers.push_back(listener);
  }
  d_notifySubscribers.push_back(listener);
}

void VariableDatabase::performGC(const std::set<Variable>& varsToKeep, VariableRelocationInfo& relocationInfo) {

  // Clear any relocation info
  relocationInfo.clear();

  // The only ones we are actually relocating
  std::vector< std::vector<Node> > variableNodesNew;
  node_to_variable_map nodeToVariableMapNew;

  // We don't GC Types, so the type-sizes stay
  variableNodesNew.resize(d_variableNodes.size());

  std::set<Variable>::const_iterator it = varsToKeep.begin();
  std::set<Variable>::const_iterator it_end = varsToKeep.end();
  for (; it != it_end; ++ it) {
    Variable oldVar = *it;
    Node oldVarNode = getNode(oldVar);
    // Relocate
    size_t typeIndex = oldVar.typeIndex();
    size_t newVarId = variableNodesNew[typeIndex].size();
    variableNodesNew[typeIndex].push_back(oldVarNode);
    // New Variable
    Variable newVar(newVarId, typeIndex);
    // Node map info
    nodeToVariableMapNew[oldVarNode] = newVar;
    // Record
    relocationInfo.add(oldVar, newVar);
  }

  // Finally swap out the old data with the new one
  d_variableNodes.swap(variableNodesNew);
  d_nodeToVariableMap.swap(nodeToVariableMapNew);
}

void VariableRelocationInfo::add(Variable oldVar, Variable newVar) {
  Assert(d_map.find(oldVar) == d_map.end());
  d_map[oldVar] = newVar;
  // Remember in the proper list
  size_t typeIndex = oldVar.typeIndex();
  if (typeIndex >= d_oldByType.size()) {
    d_oldByType.resize(typeIndex + 1);
  }
  d_oldByType[typeIndex].push_back(RelocationPair(oldVar, newVar));
}

Variable VariableRelocationInfo::relocate(Variable oldVar) const {
  relocation_map::const_iterator find = d_map.find(oldVar);
  if (find == d_map.end()) return Variable();
  else {
    return find->second;
  }
}

void VariableRelocationInfo::relocate(std::vector<Variable>& variables) const {
  for (unsigned i = 0; i < variables.size(); ++ i) {
    variables[i] = relocate(variables[i]);
  }
}

/** Iterator to */
VariableRelocationInfo::const_iterator VariableRelocationInfo::begin(size_t typeIndex) const {
  return d_oldByType[typeIndex].begin();
}

/** Iterator to */
VariableRelocationInfo::const_iterator VariableRelocationInfo::end(size_t typeIndex) const {
  return d_oldByType[typeIndex].end();
}
