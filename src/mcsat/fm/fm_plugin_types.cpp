#include "mcsat/fm/fm_plugin_types.h"

using namespace CVC4;
using namespace mcsat;
using namespace fm;
 
CDBoundsModel::CDBoundsModel(context::Context* context)
: context::ContextNotifyObj(context)
, d_boundTrailSize(context, 0)
{
}

void CDBoundsModel::contextNotifyPop() {
  
  // Go back and undo the updates 
  for (int i = d_boundTrail.size() - 1; i >= (int) d_boundTrailSize; -- i) {
    const UndoInfo& undo = d_boundTrailUndo[i];
    if (undo.isLower) {
      bound_map::iterator find = d_lowerBounds.find(undo.var);
      if (undo.prev == null) {
	d_lowerBounds.erase(find);
      } else {
	find->second = undo.prev;
      }
    } else {
      bound_map::iterator find = d_upperBounds.find(undo.var);
      if (undo.prev == null) {
	d_upperBounds.erase(find);
      } else {
	find->second = undo.prev;
      }
    }
  }
  
  // Resize the trail to size
  d_boundTrail.resize(d_boundTrailSize);
  d_boundTrailUndo.resize(d_boundTrailSize);
}
  
void CDBoundsModel::updateLowerBound(Variable var, const BoundInfo& info) {
  // Add the new info to the trail
  BoundIndex index = d_boundTrail.size();
  d_boundTrail.push_back(info);
  
  // Update the bound and prev info
  bound_map::iterator find = d_lowerBounds.find(var);
  if (find == d_lowerBounds.end()) {
    // New bound
    d_boundTrailUndo.push_back(UndoInfo(var, null, true));
    d_lowerBounds[var] = index;  
  } else {
    d_boundTrailUndo.push_back(UndoInfo(var, find->second, true));
    find->second = index;
  }
  
  // Update trail size
  d_boundTrailSize = d_boundTrail.size();
}

void CDBoundsModel::updateUpperBound(Variable var, const BoundInfo& info) {
  // Add the new info to the trail
  BoundIndex index = d_boundTrail.size();
  d_boundTrail.push_back(info);
  
  // Update the bound and prev info
  bound_map::iterator find = d_upperBounds.find(var);
  if (find == d_upperBounds.end()) {
    // New bound
    d_boundTrailUndo.push_back(UndoInfo(var, null, false));
    d_upperBounds[var] = index;  
  } else {
    d_boundTrailUndo.push_back(UndoInfo(var, find->second, false));
    find->second = index;
  }
  
  // Update trail size
  d_boundTrailSize = d_boundTrail.size();
}

bool CDBoundsModel::hasLowerBound(Variable var) const {
  return d_lowerBounds.find(var) != d_lowerBounds.end();
}

bool CDBoundsModel::hasUpperBound(Variable var) const {
  return d_upperBounds.find(var) != d_upperBounds.end();
}

const BoundInfo& CDBoundsModel::getLowerBoundInfo(Variable var) const {
  Assert(hasLowerBound(var));
  return d_boundTrail[d_lowerBounds.find(var)->second];
}

const BoundInfo& CDBoundsModel::getUpperBoundInfo(Variable var) const {
  Assert(hasUpperBound(var));
  return d_boundTrail[d_upperBounds.find(var)->second];
}

void LinearConstraint::getVariables(std::vector<Variable>& vars) {
  var_to_rational_map::const_iterator it = coefficients.begin();
  var_to_rational_map::const_iterator it_end = coefficients.end();
  while (it != it_end) {
    Variable var = it->first;
    if (!var.isNull()) {
      vars.push_back(var);
    }
    ++ it;
  }
}

void LinearConstraint::swap(LinearConstraint& c) {
  coefficients.swap(c.coefficients);
  std::swap(kind, c.kind);
}
