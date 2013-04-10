#include "mcsat/bcp/bcp_engine.h"
#include "mcsat/options.h"

using namespace CVC4;
using namespace mcsat;

BCPEngine::NewClauseNotify::NewClauseNotify(BCPEngine& engine)
: ClauseDatabase::INewClauseNotify(false)
, d_engine(engine)
{}

void BCPEngine::NewClauseNotify::newClause(CRef cref) {
  d_engine.newClause(cref);
};

BCPEngine::NewVariableNotify::NewVariableNotify(BCPEngine& engine)
: INewVariableNotify(false)
, d_engine(engine)
, d_bool_type_index(VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->booleanType()))
{}

void BCPEngine::NewVariableNotify::newVariable(Variable var) {
  if (var.typeIndex() == d_bool_type_index) {
    d_engine.newVariable(var);
  }
}

BCPEngine::BCPEngine(ClauseDatabase& clauseDb, const SolverTrail& trail, SolverPluginRequest& request)
: SolverPlugin(clauseDb, trail, request)
, d_newClauseNotify(*this)
, d_newVariableNotify(*this)
, d_trailHead(d_trail.getSearchContext())
, d_restartsCount(0)
, d_restartBase(options::mcsat_bcp_restart_base())
, d_restartInit(options::mcsat_bcp_restart_init())
, d_conflictsCount(0)
, d_conflictsLimit(d_restartInit)
{
  // Listen to new clauses
  clauseDb.addNewClauseListener(&d_newClauseNotify);

  // Listen to new variables
  VariableDatabase& db = *VariableDatabase::getCurrentDB();
  db.addNewVariableListener(&d_newVariableNotify);
  d_boolTypeIndex = db.getTypeIndex(NodeManager::currentNM()->booleanType());

  // Features we provide
  addFeature(CAN_PROPAGATE);
  addFeature(CAN_DECIDE);

  // Notifications we care about 
  addNotification(NOTIFY_RESTART);
  addNotification(NOTIFY_CONFLICT);
  addNotification(NOTIFY_CONFLICT_RESOLUTION);
  addNotification(NOTIFY_VARIABLE_UNSET);  

  Notice() << "mcsat::BCPEngine: Variable selection: " << (options::use_mcsat_bcp_var_heuristic() ? "on" : "off") << std::endl;
  Notice() << "mcsat::BCPEngine: Variable value by phase : " << (options::use_mcsat_bcp_value_phase_heuristic() ? "on" : "off") << std::endl;  
}

class BCPNewClauseCmp {
  const SolverTrail& d_trail;
public:
  BCPNewClauseCmp(const SolverTrail& trail)
  : d_trail(trail) {}

  bool operator () (const Literal& l1, const Literal& l2) {
    TNode l1_value = d_trail.value(l1);
    TNode l2_value = d_trail.value(l2);

    // Two unassigned literals are sorted arbitrarily
    if (l1_value.isNull() && l2_value.isNull()) {
      return l1 < l2;
    }

    // Unassigned literals are put to front
    if (l1_value.isNull()) return true;
    if (l2_value.isNull()) return false;

    // Literals of the same value are sorted by decreasing levels
    if (l1_value == l2_value) {
      Variable l1_var = l1.getVariable();
      Variable l2_var = l2.getVariable();
      unsigned l1_level = d_trail.decisionLevel(l1_var);
      unsigned l2_level = d_trail.decisionLevel(l2_var);
      if (l1_level != l2_level) {
        return l1_level > l2_level;
      } else {
        return d_trail.trailIndex(l1_var) > d_trail.trailIndex(l2_var);
      }
    } else {
      // Of two assigned literals, the true literal goes up front
      return (l1_value == d_trail.getTrue());
    }
  }
};

void BCPEngine::newClause(CRef cRef) {
  Debug("mcsat::bcp") << "BCPEngine::newClause(" << cRef << ")" << std::endl;
  Clause& clause = cRef.getClause();
  if (clause.size() == 1) {
    if (d_trail.decisionLevel() > 0) {
      d_request.backtrack(0, cRef);
    } else {
      d_delayedPropagations.push_back(cRef);
    }
  } else {

    BCPNewClauseCmp cmp(d_trail);
    clause.sort(cmp);

    Debug("mcsat::bcp") << "BCPEngine::newClause(): sorted: " << cRef << std::endl;

    // Attach the top two literals
    bool attach = options::mcsat_bcp_attach_limit() == 0 || clause.size() <= options::mcsat_bcp_attach_limit();
    if (attach) {
      d_watchManager.add(~clause[0], cRef);
      d_watchManager.add(~clause[1], cRef);
    }
      
    // If clause[1] is false, the clause propagates
    if (d_trail.isFalse(clause[1])) {
      Debug("mcsat::bcp") << "BCPEngine::newClause(): false at creation" << std::endl;
      unsigned propagationLevel = d_trail.decisionLevel(clause[1].getVariable());
      if (propagationLevel < d_trail.decisionLevel()) {
        d_request.backtrack(propagationLevel, cRef);
      } else {
        d_delayedPropagations.push_back(cRef);
      }
    }
  }
}

void BCPEngine::newVariable(Variable var) {

  Debug("mcsat::bcp") << "BCPEngine::newVariable(" << var << ")" << std::endl;

  // New variable
  d_variableValues.resize(var.index() + 1, false);

  // Add to the queue
  d_variableQueue.newVariable(var);
}

void BCPEngine::propagate(SolverTrail::PropagationToken& out) {

  Debug("mcsat::bcp") << "BCPEngine::propagate()" << std::endl;

  // Propagate the unit clauses
  for (unsigned i = 0; i < d_delayedPropagations.size(); ++ i) {
    out(d_delayedPropagations[i].getClause()[0], d_delayedPropagations[i]);
  }
  d_delayedPropagations.clear();

  // Make sure the watches are clean
  d_watchManager.clean();
  
  // Propagate
  unsigned i = d_trailHead;
  for (; d_trail.consistent() && i < d_trail.size(); ++ i) {
    Variable var = d_trail[i].var;
    
    if (var.typeIndex() == d_boolTypeIndex) {
      
      // Variable that was propagated
      bool var_value = d_trail.isTrue(var);

      // The literal that was propagated
      Literal lit(var, !var_value);
      // Negation of the literal (one that we're looking for in clauses)
      Literal lit_neg = ~lit;
    
      // Remember the value
      d_variableValues[var.index()] = var_value;
    
      Debug("mcsat::bcp") << "BCPEngine::propagate(): propagating on " << lit << " with value " << d_trail.value(lit) << std::endl;

      // Get the watchlist of the literal to propagate
      WatchListManager::remove_iterator w = d_watchManager.begin(lit);
      
      // Propagate through the watchlist
      while (d_trail.consistent() && !w.done()) {
	// Get the watched clause
	CRef cRef = *w;

        Debug("mcsat::bcp") << "BCPEngine::propagate(): propagating over " << cRef << std::endl;

	Clause& clause = cRef.getClause();

	// If clause is not in use anymore, just skip it
	if (!clause.inUse()) {
	  Debug("mcsat::bcp") << "BCPEngine::propagate(): clause " << clause << " not in use anymore";
	  // Remove this watch and go to the next one 
	  w.next_and_remove();
	  continue;
	}
	
	// Put the propagation literal to position [1]
	if (clause[0] == lit_neg) {
	  clause.swapLiterals(0, 1);
	}
        
        // If 0th literal is true, the clause is already satisfied.
	if (d_trail.isTrue(clause[0])) {
          Debug("mcsat::bcp") << "BCPEngine::propagate(): clause " << clause << " is true";
	  // Keep this watch and go to the next one 
	  w.next_and_keep();
	  continue;
	}

	// Try to find a new watch
	bool watchFound = false;
        for (unsigned j = 2; j < clause.size(); j++) {
          Debug("mcsat::bcp") << "BCPEngine::propagate(): checking " << clause[j] << std::endl;
          if (!d_trail.isFalse(clause[j])) {
	    // Put the new watch in place
	    clause.swapLiterals(1, j);
	    d_watchManager.add(~clause[1], cRef);
	    w.next_and_remove();
	    // We found a watch
	    watchFound = true;
	    break;
	  }
	}

        Debug("mcsat::bcp") << "BCPEngine::propagate(): found watch = " << (watchFound ? "true" : "false") << std::endl;

        if (!watchFound) {
	  // We did not find a watch so clause[0] is propagated to be true
	  out(clause[0], cRef);
	
	  // Keep this watch 
	  w.next_and_keep(); 
	}
      }
    }
  }

  /** Update the CD trail head */
  d_trailHead = i;

  Debug("mcsat::bcp") << "BCPEngine::propagate(): done" << std::endl;

}

void BCPEngine::decide(SolverTrail::DecisionToken& out) {
  Debug("mcsat::bcp") << "BCPEngine::decide()" << std::endl;
  Assert(d_delayedPropagations.size() == 0 && d_trailHead == d_trail.size());
  while (!d_variableQueue.empty()) {
    Variable var = d_variableQueue.pop();

    if (!d_trail.hasValue(var)) {
      
      // Phase heuristic
      if (options::use_mcsat_bcp_value_phase_heuristic()) {
	if (d_variableValues[var.index()]) {
	  out(Literal(var, false));
	} else {
	  out(Literal(var, true));
	}
	return;
      } 
      
      // No heuristic, just decide !var first
      out(Literal(var, true));
      
      return;
    }
  }
}

void BCPEngine::notifyVariableUnset(const std::vector<Variable>& vars) {
  // Just add the Boolean variables to the queue
  for (unsigned i = 0; i < vars.size(); ++ i) {
    if (vars[i].typeIndex() == d_boolTypeIndex) {
      d_variableQueue.enqueue(vars[i]);
    }
  }
}

static unsigned luby(unsigned index) {

    unsigned size = 1;
    unsigned maxPower = 0;

    // Find the first full subsequence such that total size covers the index
    while (size <= index) {
        size = 2*size + 1;
        ++ maxPower;
    }

    // Now get the exact value
    while (size > index + 1) {
        size = size / 2;
        -- maxPower;
        if (size <= index) {
            index -= size;
        }
    }

    return maxPower + 1;
}

void BCPEngine::notifyConflict() {
  d_conflictsCount ++;
  if (d_conflictsCount >= d_conflictsLimit) {
    d_request.restart();
  }
}

void BCPEngine::notifyConflictResolution(CRef cRef) {
  // The clause that is resolved
  Clause& clause = cRef.getClause();
  
  if (options::use_mcsat_bcp_var_heuristic()) {
    // Bump each variable that is resolved
    for (unsigned i = 0; i < clause.size(); ++ i) {
      d_variableQueue.bumpVariable(clause[i].getVariable());
    }
  }
}
  
void BCPEngine::notifyRestart() {
  d_restartsCount ++;
  d_conflictsCount = 0;
  d_conflictsLimit = d_restartInit * pow(d_restartBase, luby(d_restartsCount));
}
