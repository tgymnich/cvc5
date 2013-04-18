#include "mcsat/fm/fm_plugin_types.h"

using namespace CVC4;
using namespace mcsat;
using namespace fm;

CDBoundsModel::CDBoundsModel(context::Context* context)
: context::ContextNotifyObj(context)
, d_boundTrailSize(context, 0)
, d_disequalTrailSize(context, 0)
{
}

void CDBoundsModel::contextNotifyPop() {
  
  // Go back and undo the updates 
  for (int i = d_boundTrail.size() - 1; i >= (int) d_boundTrailSize; -- i) {
    const UndoBoundInfo& undo = d_boundTrailUndo[i];
    if (undo.isLower) {
      bound_map::iterator find = d_lowerBounds.find(undo.var);
      if (undo.prev == null_bound_index) {
	d_lowerBounds.erase(find);
      } else {
	find->second = undo.prev;
      }
    } else {
      bound_map::iterator find = d_upperBounds.find(undo.var);
      if (undo.prev == null_bound_index) {
	d_upperBounds.erase(find);
      } else {
	find->second = undo.prev;
      }
    }
  }
  
  // Resize the trail to size
  d_boundTrail.resize(d_boundTrailSize);
  d_boundTrailUndo.resize(d_boundTrailSize);

  // Go back and undo the updates
  for (int i = d_disequalTrail.size() - 1; i >= (int) d_disequalTrailSize; -- i) {
    const UndoDisequalInfo& undo = d_disequalTrailUndo[i];
    disequal_map::iterator find = d_disequalValues.find(undo.var);
    if (undo.prev == null_bound_index) {
      d_disequalValues.erase(find);
    } else {
      find->second = undo.prev;
    }
  }

  // Resize the trail to size
  d_disequalTrail.resize(d_disequalTrailSize);
  d_disequalTrailUndo.resize(d_disequalTrailSize);

  // Clear any conflicts 
  d_variablesInConflict.clear();
}

void CDBoundsModel::addDisequality(Variable var, const DisequalInfo& info) {

  // If the value is outside of the feasible range, we can ignore it
  bool onlyOption = false;
  if (inRange(var, info.value, onlyOption)) {

    Debug("mcsat::fm") << "CDBoundsModel::addDiseq(" << var << ", " << info << ")" << std::endl;

    // Add the new info to the trail
    DisequalInfoIndex index = d_disequalTrail.size();
    d_disequalTrail.push_back(info);

    disequal_map::iterator find = d_disequalValues.find(var);
    if (find == d_disequalValues.end()) {
      d_disequalTrailUndo.push_back(UndoDisequalInfo(var, null_diseqal_index));
      d_disequalValues[var] = index;
    } else {
      d_disequalTrailUndo.push_back(UndoDisequalInfo(var, find->second));
      find->second = index;
    }

    // Update the trail size
    d_disequalTrailSize = d_disequalTrail.size();

    if (onlyOption) {
      // Conflict
      d_variablesInConflict.insert(var);
      Debug("mcsat::fm") << "CDBoundsModel::addDiseq(" << var << ", " << info << "): bound and diseq conflict" << std::endl;
    }
  }
}

bool CDBoundsModel::updateLowerBound(Variable var, const BoundInfo& lBound) {
  // Update if better than the current one
  if (!hasLowerBound(var) || lBound.improvesLowerBound(getLowerBoundInfo(var))) {  
    
    Debug("mcsat::fm") << "CDBoundsModel::updateLowerBound(" << var << ", " << lBound << ")" << std::endl;

    // Add the new info to the trail
    BoundInfoIndex index = d_boundTrail.size();
    d_boundTrail.push_back(lBound);
  
    // Update the bound and prev info
    bound_map::iterator find = d_lowerBounds.find(var);
    if (find == d_lowerBounds.end()) {
      // New bound
      d_boundTrailUndo.push_back(UndoBoundInfo(var, null_bound_index, true));
      d_lowerBounds[var] = index;  
    } else {
      d_boundTrailUndo.push_back(UndoBoundInfo(var, find->second, true));
      find->second = index;
    }
  
    // Update trail size
    d_boundTrailSize = d_boundTrail.size();

    // Mark if new bound in conflict
    if (hasUpperBound(var)) {
      const BoundInfo& uBound = getUpperBoundInfo(var);
      // Bounds in conflict
      if (BoundInfo::inConflict(lBound, uBound)) {
        d_variablesInConflict.insert(var);
        Debug("mcsat::fm") << "CDBoundsModel::updateLowerBound(" << var << ", " << lBound << "): bound conflict" << std::endl;
      }
      // Bounds imply a value that is in the disequal list
      else if (!lBound.strict && !uBound.strict && lBound.value == uBound.value) {
        if (isDisequal(var, lBound.value)) {
          d_variablesInConflict.insert(var);
          Debug("mcsat::fm") << "CDBoundsModel::updateLowerBound(" << var << ", " << lBound << "): bound and diseq conflict" << std::endl;
        }
        // Value for var is now fixed
        return true;
      }
    }
  }

  return false;
}

bool CDBoundsModel::updateUpperBound(Variable var, const BoundInfo& uBound) {
  if (!hasUpperBound(var) || uBound.improvesUpperBound(getUpperBoundInfo(var))) {

    Debug("mcsat::fm") << "CDBoundsModel::updateUpperBound(" << var << ", " << uBound << ")" << std::endl;

    // Add the new info to the trail
    BoundInfoIndex index = d_boundTrail.size();
    d_boundTrail.push_back(uBound);
  
    // Update the bound and prev info
    bound_map::iterator find = d_upperBounds.find(var);
    if (find == d_upperBounds.end()) {
      // New bound
      d_boundTrailUndo.push_back(UndoBoundInfo(var, null_bound_index, false));
      d_upperBounds[var] = index;  
    } else {
      d_boundTrailUndo.push_back(UndoBoundInfo(var, find->second, false));
      find->second = index;
    }
  
    // Update trail size
    d_boundTrailSize = d_boundTrail.size();

    // Mark if new bound in conflict
    if (hasLowerBound(var)) {
      const BoundInfo& lBound = getLowerBoundInfo(var);
      // Bounds in conflictr
      if (BoundInfo::inConflict(lBound, uBound)) {
        Debug("mcsat::fm") << "CDBoundsModel::updateUpperBound(" << var << ", " << uBound << "): bound conflict" << std::endl;
        d_variablesInConflict.insert(var);
      }
      // Bounds imply a value that is in the disequal list
      else if (!lBound.strict && !uBound.strict && lBound.value == uBound.value) {
        if (isDisequal(var, lBound.value)) {
          d_variablesInConflict.insert(var);
          Debug("mcsat::fm") << "CDBoundsModel::updateUpperBound(" << var << ", " << uBound << "): bound and diseq conflict" << std::endl;
        }
        // Value for var is now fixed
        return true;
      }
    }
  }  

  return false;
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

bool CDBoundsModel::inRange(Variable var, Rational value, bool& onlyOption) const {

  bool onUpperBound = false;
  bool onLowerBound = false;

  if (hasUpperBound(var)) {
    const BoundInfo& info = getUpperBoundInfo(var);
    if (info.strict) {
      if (value >= info.value) {
        return false;
      }
    } else {
      int cmp = value.cmp(info.value);
      if (cmp > 0) {
        return false;
      } else if (cmp == 0) {
        onUpperBound = true;
      }
    }
  }

  if (hasLowerBound(var)) {
    const BoundInfo& info = getLowerBoundInfo(var);
    if (info.strict) {
      if (value <= info.value) {
        return false;
      }
    } else {
      int cmp = value.cmp(info.value);
      if (cmp < 0) {
        return false;
      } else if (cmp == 0) {
        onLowerBound = true;
      }
    }
  }

  // It's the only option if on both bounds (reals)
  onlyOption = onUpperBound && onLowerBound;

  return true;
}

Rational CDBoundsModel::pick(Variable var) const {

  Debug("mcsat::fm") << "CDBoundsModel::pick(" << var << ")" << std::endl;

  // Set of values to be disequal from
  std::set<Rational> disequal;
  getDisequal(var, disequal);

  if (hasLowerBound(var)) {
    const BoundInfo& lowerBound = getLowerBoundInfo(var);
    if (hasUpperBound(var)) {
      const BoundInfo& upperBound = getUpperBoundInfo(var);
      // Both bounds present, go for middle
      Rational value = (lowerBound.value + upperBound.value)/2;
      std::set<Rational>::const_iterator find = disequal.find(value);
      while (find != disequal.end()) {
        if (value < upperBound.value) {
          value = (value + upperBound.value) / 2;
        } else if (value > lowerBound.value) {
          value = (value = lowerBound.value) / 2;
        } else {
          Unreachable();
        }
        find = disequal.find(value);
      }
      return value;
    } else {
      // No upper bound, get max of (lower, diseq) and round up
      Rational max = lowerBound.value;
      if (disequal.size() > 0) {
        max = std::max(max, *disequal.rbegin());
      }
      return (max + 1).floor();
    }
  } else {
    // No lower bound
    if (hasUpperBound(var)) {
      const BoundInfo& upperBound = getUpperBoundInfo(var);
      // No lower bound, get min of (upper, diseq) and round down
      Rational min = upperBound.value;
      if (disequal.size() > 0) {
        min = std::min(min, *disequal.begin());
      }
      return (min - 1).ceiling();
    } else {
      // No bounds at all, just find 0... thats not in diseq
      unsigned x = 0;
      while (disequal.count(x) > 0) {
        x ++;
      }
      return x;
    }
  }
}

bool CDBoundsModel::isDisequal(Variable var, Rational value) const {

  disequal_map::const_iterator find = d_disequalValues.find(var);
  if (find != d_disequalValues.end()) {
    // Go through all the values and check for the value
    DisequalInfoIndex it = find->second;
    while (it != null_diseqal_index) {
      const DisequalInfo& info = d_disequalTrail[it];
      if (info.value == value) {
        return true;
      } else {
        it = d_disequalTrailUndo[it].prev;
      }
    }
    // Seen all disequal values
    return false;
  } else {
    // Not asserted to be disequal to anything
    return false;
  }
}

void CDBoundsModel::getDisequal(Variable var, std::set<Rational>& disequal) const {
  disequal_map::const_iterator find = d_disequalValues.find(var);
  if (find != d_disequalValues.end()) {
    bool forced = false;
    // Go through all the values and check for the value
    Debug("mcsat::fm") << var << " !=";
    DisequalInfoIndex it = find->second;
    while (it != null_diseqal_index) {
      if (inRange(var, d_disequalTrail[it].value, forced)) {
        Assert(!forced);
        disequal.insert(d_disequalTrail[it].value);
	Debug("mcsat::fm") << " " << d_disequalTrail[it].value;
      }
      it = d_disequalTrailUndo[it].prev;
    }
    Debug("mcsat::fm") << std::endl;
  }
}

const DisequalInfo& CDBoundsModel::getDisequalInfo(Variable var, Rational value) const {

  DisequalInfoIndex found_index = null_diseqal_index;

  disequal_map::const_iterator find = d_disequalValues.find(var);
  if (find != d_disequalValues.end()) {
    // Go through all the values and check for the value
    Debug("mcsat::fm") << var << " !=";
    DisequalInfoIndex it = find->second;
    while (it != null_diseqal_index) {
      if (d_disequalTrail[it].value == value) {
        found_index = it;
      }
      it = d_disequalTrailUndo[it].prev;
    }
    Debug("mcsat::fm") << std::endl;
  }

  Assert(found_index != null_diseqal_index);
  return d_disequalTrail[found_index];
}
