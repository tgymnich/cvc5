#include "mcsat/fm/fm_plugin.h"
#include "mcsat/options.h"

using namespace CVC4;
using namespace mcsat;
using namespace fm;
using namespace util;

FMPlugin::NewVariableNotify::NewVariableNotify(FMPlugin& plugin)
: INewVariableNotify(false)
, d_plugin(plugin)
{
}

void FMPlugin::NewVariableNotify::newVariable(Variable var) {
  // Register new variables
  if (d_plugin.isArithmeticVariable(var)) {
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
, d_intTypeIndex(VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->integerType()))
, d_realTypeIndex(VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->realType()))
, d_fixedVariables(trail.getSearchContext())
, d_fixedVariablesIndex(trail.getSearchContext(), 0)
, d_fixedVariablesDecided(trail.getSearchContext(), 0)
, d_trailHead(trail.getSearchContext(), 0)
, d_bounds(trail.getSearchContext())
, d_fmRule(database, trail)
, d_variableQueue()
{
  Debug("mcsat::fm") << "FMPlugin::FMPlugin()" << std::endl;

  // We decide values of variables and can detect conflicts
  addFeature(CAN_PROPAGATE);
  addFeature(CAN_DECIDE_VALUES);

  // Notifications we need
  addNotification(NOTIFY_BACKJUMP);

  // Add the listener
  VariableDatabase::getCurrentDB()->addNewVariableListener(&d_newVariableNotify);
}

std::string FMPlugin::toString() const  {
  return "Fourier-Motzkin Elimination (Model-Based)";
}

void FMPlugin::newVariable(Variable var) {
  Debug("mcsat::fm") << "FMPlugin::newVariable(" << var << ")" << std::endl;
  d_variableQueue.newVariable(var);
}

class var_assign_compare {
  const SolverTrail& d_trail;
public:
  var_assign_compare(const SolverTrail& trail)
  : d_trail(trail) {}

  bool operator () (const Variable& v1, const Variable& v2) {

    bool v1_hasValue = d_trail.hasValue(v1);
    bool v2_hasValue = d_trail.hasValue(v2);

    // Two unassigned literals are sorted arbitrarily
    if (!v1_hasValue && !v2_hasValue) {
      return v1 < v2;
    }

    // Unassigned variables are put to front
    if (!v1_hasValue) return true;
    if (!v2_hasValue) return false;

    // Both are assigned, we compare by trail index
    return d_trail.decisionLevel(v1) > d_trail.decisionLevel(v2);
  }
};

void FMPlugin::newConstraint(Variable constraint) {
  Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << ")" << std::endl;
  Assert(!isLinearConstraint(constraint), "Already registered");

  // The linear constraint we're creating 
  LinearConstraint linearConstraint;

  // Parse the coefficients into the constraint
  bool linear = LinearConstraint::parse(Literal(constraint, false), linearConstraint);
  if (!linear) {
    // The plugin doesn't handle non-linear constraints
    Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << "): non-linear" << std::endl;
    return;
  }

  Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << "): " << linearConstraint << std::endl;

  // Get the variables of the constraint
  std::vector<Variable> vars;
  linearConstraint.getVariables(vars);
  Assert(vars.size() > 0);

  // Sort the variables by trail assignment index (if any)
  var_assign_compare cmp(d_trail);
  std::sort(vars.begin(), vars.end(), cmp);

  // Add the variable list to the watch manager and watch the first two constraints
  VariableListReference listRef = d_assignedWatchManager.newListToWatch(vars, constraint);
  VariableList list = d_assignedWatchManager.get(listRef);
  Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << "): new list " << list << std::endl;

  d_assignedWatchManager.watch(vars[0], listRef);
  if (vars.size() > 1) {
    d_assignedWatchManager.watch(vars[1], listRef);
  }
  
  // Remember the constraint
  d_constraints[constraint].swap(linearConstraint);
  // Status of the constraint
  if (constraint.index() >= d_constraintUnassignedStatus.size()) {
    d_constraintUnassignedStatus.resize(constraint.index() + 1, UNASSIGNED_UNKNOWN);
  }

  // Check if anything to do immediately
  if (!d_trail.hasValue(vars[0])) {
    if (vars.size() == 1 || d_trail.hasValue(vars[1])) {
      // Single variable is unassigned
      d_constraintUnassignedStatus[constraint.index()] = UNASSIGNED_UNIT;
      Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << "): unit " << std::endl;
    }
  } else {
    // All variables are unassigned    
    d_constraintUnassignedStatus[constraint.index()] = UNASSIGNED_NONE;
    Debug("mcsat::fm") << "FMPlugin::newConstraint(" << constraint << "): all assigned " << std::endl;
    // Propagate later
    d_delayedPropagations.push_back(constraint);
  }
}

void FMPlugin::propagate(SolverTrail::PropagationToken& out) {
  Debug("mcsat::fm") << "FMPlugin::propagate()" << std::endl;

  unsigned i = d_trailHead;
  for (; i < d_trail.size() && d_trail.consistent(); ++ i) {
    Variable var = d_trail[i].var;

    if (isArithmeticVariable(var)) {

      Debug("mcsat::fm") << "FMPlugin::propagate(): " << var << " assigned" << std::endl;

      // Go through all the variable lists (constraints) where we're watching var
      AssignedWatchManager::remove_iterator w = d_assignedWatchManager.begin(var);
      while (d_trail.consistent() && !w.done()) {

        // Get the current list where var appears
        VariableListReference variableListRef = *w;
        VariableList variableList = d_assignedWatchManager.get(variableListRef);
        Debug("mcsat::fm") << "FMPlugin::propagate(): watchlist  " << variableList << " assigned" << std::endl;

        // More than one vars, put the just assigned var to position [1]
        if (variableList.size() > 1 && variableList[0] == var) {
          variableList.swap(0, 1);
        }

        // Try to find a new watch
        bool watchFound = false;
        for (unsigned j = 2; j < variableList.size(); j++) {
          if (!d_trail.hasValue(variableList[j])) {
            // Put the new watch in place
            variableList.swap(1, j);
            d_assignedWatchManager.watch(variableList[1], variableListRef);
            w.next_and_remove();
            // We found a watch
            watchFound = true;
            break;
          }
        }

        Debug("mcsat::fm") << "FMPlugin::propagate(): new watch " << (watchFound ? "found" : "not found") << std::endl;

        if (!watchFound) {
          // We did not find a new watch so vars[1], ..., vars[n] are assigned, except maybe vars[0]
          if (d_trail.hasValue(variableList[0])) {
            // Even the first one is assigned, so we have a semantic propagation
            Variable constraintVar = d_assignedWatchManager.getConstraint(variableListRef);
            unsigned valueLevel;
            if (!d_trail.hasValue(constraintVar)) {
              const LinearConstraint& constraint = getLinearConstraint(constraintVar);
              bool value = constraint.evaluate(d_trail, valueLevel);
              out(Literal(constraintVar, !value), valueLevel);
            } else {
              Assert(getLinearConstraint(constraintVar).evaluate(d_trail, valueLevel) == d_trail.isTrue(constraintVar));
            }
            // Mark the assignment
            d_constraintUnassignedStatus[constraintVar.index()] = UNASSIGNED_NONE;
          } else {
            // The first one is not assigned, so we have a new unit constraint
            Variable constraintVar = d_assignedWatchManager.getConstraint(variableListRef);
            d_constraintUnassignedStatus[constraintVar.index()] = UNASSIGNED_UNIT;
            // If the constraint was already asserted, then process it
            if (d_trail.hasValue(constraintVar)) {
              processUnitConstraint(constraintVar);
            }
          }
          // Keep the watch, and continue
          w.next_and_keep();
        }
      }
    } else if (isLinearConstraint(var)) {
      // If the constraint is unit we propagate a new bound
      if (isUnitConstraint(var)) {
	Debug("mcsat::fm") << "FMPlugin::propagate(): processing " << var << " -> " << d_trail.value(var) << std::endl;
        processUnitConstraint(var);
      } else {
	Debug("mcsat::fm") << "FMPlugin::propagate(): " << var << " not unit yet" << std::endl;
      }
    }
  }

  // Remember where we stopped
  d_trailHead = i;

  // Process any conflicts
  if (d_bounds.inConflict()) {
    processConflicts(out);
  }
}

bool FMPlugin::isUnitConstraint(Variable constraint) {
  return d_constraintUnassignedStatus[constraint.index()] == UNASSIGNED_UNIT;
}

void FMPlugin::processUnitConstraint(Variable constraint) {
  Debug("mcsat::fm") << "FMPlugin::processUnitConstraint(" << constraint << ")" << std::endl;
  Assert(isLinearConstraint(constraint));
  Assert(isUnitConstraint(constraint));

  // Get the constraint
  const LinearConstraint& c = getLinearConstraint(constraint);
  Debug("mcsat::fm") << "FMPlugin::processUnitConstraint(): " << c << " with value " << d_trail.value(constraint) << std::endl;

  // Compute ax + sum:
  // * the sum of the linear term that evaluates
  // * the single variable that is unassigned
  // * coefficient of the variable
  Rational sum;
  Rational a;
  Variable x;

  LinearConstraint::const_iterator it = c.begin();
  LinearConstraint::const_iterator it_end = c.end();
  for (; it != it_end; ++ it) {
    Variable var = it->first;
    // Constant term
    if (var.isNull()) {
      sum += it->second;
      continue;
    }
    // Assigned variable
    if (d_trail.hasValue(var)) {
      Rational varValue = d_trail.value(var).getConst<Rational>();
      sum += it->second * varValue;
    } else {
      Assert(x.isNull());
      x = var;
      a = it->second;
    }
  }

  // The constraint kind
  Kind kind = c.getKind();
  if (d_trail.isFalse(constraint)) {
    // If the constraint is negated, negate the kind
    kind = LinearConstraint::negateKind(kind);
  }
  if (a < 0) {
    // If a is negative flip the kind
    kind = LinearConstraint::flipKind(kind);
    a = -a;
    sum = -sum;
  }

  // Do the work (assuming a > 0)
  Rational value = -sum/a;
  bool fixed = false;
  switch (kind) {
  case kind::GT:
  case kind::GEQ:
    // (ax + sum >= 0) <=> (x >= -sum/a)
    fixed = d_bounds.updateLowerBound(x, BoundInfo(value, kind == kind::GT, constraint));
    break;
  case kind::LT:
  case kind::LEQ:
    // (ax + sum <= 0) <=> (x <= -sum/a)
    fixed = d_bounds.updateUpperBound(x, BoundInfo(value, kind == kind::LT, constraint));
    break;
  case kind::EQUAL:
    // (ax + sum == 0) <=> (x >= -sum/a) and (x <= -sum/a)
    d_bounds.updateLowerBound(x, BoundInfo(value, false, constraint));
    d_bounds.updateUpperBound(x, BoundInfo(value, false, constraint));
    fixed = true;
    break;
  case kind::DISTINCT:
    // (ax + sum != 0) <=> x != -sum/a
    d_bounds.addDisequality(x, DisequalInfo(value, constraint));
    break;
  default:
    Unreachable();
  }

  if (fixed) {
    d_fixedVariables.push_back(x);
  }
}

void FMPlugin::processConflicts(SolverTrail::PropagationToken& out) {
  std::set<Variable> variablesInConflict;
  d_bounds.getVariablesInConflict(variablesInConflict);

  std::set<Variable>::const_iterator it = variablesInConflict.begin();
  std::set<Variable>::const_iterator it_end = variablesInConflict.end();
  for (; it != it_end; ++ it) {
    Variable var = *it;

    const BoundInfo& lowerBound = d_bounds.getLowerBoundInfo(var);
    const BoundInfo& upperBound = d_bounds.getUpperBoundInfo(var);

    CRef conflict;
    if (BoundInfo::inConflict(lowerBound, upperBound)) {
      // Clash of bounds
      Variable lowerBoundVariable = lowerBound.reason;
      Literal lowerBoundLiteral(lowerBoundVariable, d_trail.isFalse(lowerBoundVariable));
      Variable upperBoundVariable = upperBound.reason;
      Literal upperBoundLiteral(upperBoundVariable, d_trail.isFalse(upperBoundVariable));

      d_fmRule.start(lowerBoundLiteral);
      d_fmRule.resolve(var, upperBoundLiteral);
      conflict = d_fmRule.finish(out);
    } else {
      // Bounds clash with a disequality
      Variable lowerBoundVariable = lowerBound.reason;
      Literal lowerBoundLiteral(lowerBoundVariable, d_trail.isFalse(lowerBoundVariable));
      Variable upperBoundVariable = upperBound.reason;
      Literal upperBoundLiteral(upperBoundVariable, d_trail.isFalse(upperBoundVariable));

      Assert (!lowerBound.strict && !upperBound.strict && lowerBound.value == upperBound.value);
      const DisequalInfo& disequal = d_bounds.getDisequalInfo(var, lowerBound.value);
      Literal disequalityLiteral(disequal.reason, d_trail.isFalse(disequal.reason));

      conflict = d_fmRule.resolveDisequality(var, lowerBoundLiteral, upperBoundLiteral, disequalityLiteral, out);
    }

    Clause& conflictClause = conflict.getClause();
    std::vector<Variable> conflictVars;
    for (unsigned i = 0; i < conflictClause.size(); ++ i) {
      if (isLinearConstraint(conflictClause[i].getVariable())) {
        getLinearConstraint(conflictClause[i].getVariable()).getVariables(conflictVars);
      }
    }
    for (unsigned i = 0; i < conflictVars.size(); ++ i) {
      d_variableQueue.bumpVariable(conflictVars[i]);
    }
  }
}

void FMPlugin::decide(SolverTrail::DecisionToken& out) {
  Debug("mcsat::fm") << "FMPlugin::decide(): level " << d_trail.decisionLevel() << std::endl;
  Assert(d_trailHead == d_trail.size());

  for (; d_fixedVariablesIndex < d_fixedVariables.size(); d_fixedVariablesIndex = d_fixedVariablesIndex + 1) {
    Variable var = d_fixedVariables[d_fixedVariablesIndex];
    if (!d_trail.hasValue(var)) {
      Rational value = d_bounds.pick(var, false);
      Debug("mcsat::fm") << "FMPlugin::decide(): [f] " << var << "[" << d_variableQueue.getScore(var) << "] -> " << value << std::endl;
      Debug("mcsat::fm::decide") << "FMPlugin::decide(): " << var << " fixed at " << d_trail.decisionLevel() << std::endl;
      out(var, NodeManager::currentNM()->mkConst(value), true);
      d_fixedVariablesDecided = d_fixedVariablesDecided + 1;
      return;
    }
  }

  if (Debug.isOn("mcsat::fm::decide")) {
    if (d_fixedVariablesDecided == d_trail.decisionLevel()) {
      Debug("mcsat::fm::decide") << "FMPlugin::decide(): initially fixed " << d_trail.decisionLevel() << std::endl; 
    }
  }
  
  while (!d_variableQueue.empty()) {
  
    Variable var; 
    
    // Do we select randomly 
    double selectRand = ((double)rand()) / RAND_MAX;
    if (selectRand < options::mcsat_fm_random_select()) {
      var = d_variableQueue.popRandom();
    } else {
      var = d_variableQueue.pop();
    }
    
//    if (var.getNode().getKind() != kind::VARIABLE) {
//      continue;
//    }

    if (d_trail.value(var).isNull()) {
      Rational value = d_bounds.pick(var, true);
      Debug("mcsat::fm") << "FMPlugin::decide(): " << var << "[" << d_variableQueue.getScore(var) << "] -> " << value << std::endl;
      out(var, NodeManager::currentNM()->mkConst(value), true);
      return;
    }
  }
}

void FMPlugin::notifyBackjump(const std::vector<Variable>& vars) {

  Debug("mcsat::fm") << "FMPlugin::notifyBackjump(): " << d_trail.decisionLevel() << std::endl;

  // Clear the delayed stuff
  d_delayedPropagations.clear();

  // Decay the scores in the queue
  d_variableQueue.decayScores();

  for (unsigned i = 0; i < vars.size(); ++ i) {
    if (isArithmeticVariable(vars[i])) {
      // Go through the watch and mark the constraints
      // UNASSIGNED_UNKNOWN -> UNASSIGNED_UNKNOWN
      // UNASSIGNED_UNIT    -> UNASSIGNED_UNKNOWN
      // UNASSIGNED_NONE    -> UNASSIGNED_UNIT
      AssignedWatchManager::remove_iterator w = d_assignedWatchManager.begin(vars[i]);
      while (!w.done()) {
        // Get the current list where var appears
        Variable constraintVar = d_assignedWatchManager.getConstraint(*w);
        switch (d_constraintUnassignedStatus[constraintVar.index()]) {
	  case UNASSIGNED_NONE:
	    d_constraintUnassignedStatus[constraintVar.index()] = UNASSIGNED_UNIT;
	    break;
	  case UNASSIGNED_UNIT:
	    if ((*w).size() > 1) {
	      // Becomes unknown only if not unit by construction
	      d_constraintUnassignedStatus[constraintVar.index()] = UNASSIGNED_UNKNOWN;
	    }
	    break;
	  case UNASSIGNED_UNKNOWN:
	    break;
	}
	w.next_and_keep();
      }

      // Also mark to be decided later
      if (!d_variableQueue.inQueue(vars[i])) {
        d_variableQueue.enqueue(vars[i]);
      }
    }
  }
}

void FMPlugin::gcMark(std::set<Variable>& varsToKeep, std::set<CRef>& clausesToKeep) {
  Assert(d_delayedPropagations.size() == 0);
  // We don't care about stuff: TODO: rethink this
}

void FMPlugin::gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc) {

  // Type of constraints 
  size_t boolType = VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->booleanType());
  
  // Go throuth deleted constraints 
  if (vReloc.size(boolType) > 0) {
    VariableGCInfo::const_iterator it = vReloc.begin(boolType);
    VariableGCInfo::const_iterator it_end = vReloc.end(boolType);
    for (; it != it_end; ++ it) {
      fm::var_to_constraint_map::iterator find = d_constraints.find(*it);
      if (find != d_constraints.end()) {
        // Remove from map: variables -> constraints
        d_constraints.erase(find);
        // Set status to unknwon
        d_constraintUnassignedStatus[it->index()] = UNASSIGNED_UNKNOWN;
      }
    }
  }

  // Relocate the watch manager
  d_assignedWatchManager.gcRelocate(vReloc);
}



