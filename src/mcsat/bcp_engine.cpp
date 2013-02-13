#include "mcsat/bcp_engine.h"

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

BCPEngine::BCPEngine(const SolverTrail& trail, SolverPluginRequest& request)
: SolverPlugin(trail, request)
, d_newClauseNotify(*this)
, d_newVariableNotify(*this)
, d_trailHead(d_trail.getSearchContext())
, d_variableScoresMax(1.0)
, d_variableScoreCmp(d_variableScores)
, d_variableQueue(d_variableScoreCmp)
{
  d_trail.addNewClauseListener(&d_newClauseNotify);
  VariableDatabase::getCurrentDB()->addNewVariableListener(&d_newVariableNotify);
}

ClauseDatabase::INewClauseNotify* BCPEngine::getNewClauseListener() {
  return &d_newClauseNotify;
}

class BCPNewClauseCmp {
  const SolverTrail& d_trail;
public:
  BCPNewClauseCmp(const SolverTrail& trail)
  : d_trail(trail) {}

  bool operator () (const Literal& l1, const Literal& l2) {
    Variable l1_value = d_trail.value(l1);
    Variable l2_value = d_trail.value(l2);

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
      return d_trail.trailIndex(l1_var) > d_trail.trailIndex(l2_var);
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
    d_watchManager.add(~clause[0], cRef);
    d_watchManager.add(~clause[1], cRef);

    // If clause[1] is false, the clause propagates
    if (d_trail.isFalse(clause[1])) {
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

  // Insert a new score (max of the current scores)
  d_variableScores.resize(var.index() + 1, d_variableScoresMax);
  // Make sure that there is enough space for the pointer
  d_variableQueuePositions.resize(var.index() + 1);
  // Add to the queue
  enqueue(var);
}

void BCPEngine::enqueue(Variable var) {
  variable_queue::point_iterator it = d_variableQueue.push(var);
  d_variableQueuePositions[var.index()] = it;
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
  
  Variable c_TRUE = d_trail.getTrue();
  Variable c_FALSE = d_trail.getFalse();

  // Type index of the Booleans
  size_t c_BOOLEAN = c_TRUE.typeIndex();

  // Propagate
  unsigned i = d_trailHead;
  for (; d_trail.consistent() && i < d_trail.size(); ++ i) {
    Variable var = d_trail[i].var;

    if (var.typeIndex() == c_BOOLEAN) {
      
      // Variable that was propagated
      Variable var_value = d_trail.value(var);
      Assert(!var_value.isNull());

      // The literal that was propagated
      Literal lit(var, var_value == c_FALSE);
      // Negation of the literal (one that we're looking for in clauses)
      Literal lit_neg = ~lit;
      
      Debug("mcsat::bcp") << "BCPEngine::propagate(): propagating on " << lit << std::endl;

      // Get the watchlist of the literal to propagate
      WatchListManager::remove_iterator w = d_watchManager.begin(lit);
      
      // Propagate through the watchlist
      while (d_trail.consistent() && !w.done()) {
	// Get the watched clause
	CRef cRef = *w;

        Debug("mcsat::bcp") << "BCPEngine::propagate(): propagating over " << cRef << std::endl;

	Clause& clause = cRef.getClause();
	
	// Put the propagation literal to position [1]
	if (clause[0] == lit_neg) {
	  clause.swapLiterals(0, 1);
	}
        
        // If 0th literal is true, the clause is already satisfied.
        if (d_trail.value(clause[0]) == c_TRUE) {
	  // Keep this watch and go to the next one 
	  w.next_and_keep();
	  continue;
	}

	// Try to find a new watch
	bool watchFound = false;
        for (unsigned j = 2; j < clause.size(); j++) {
	  if (d_trail.value(clause[j]) != c_FALSE) {
	    // Put the new watch in place
	    clause.swapLiterals(1, j);
	    d_watchManager.add(~clause[1], cRef);
	    w.next_and_remove();
	    // We found a watch
	    watchFound = true;
	  }
	}

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
    Variable var = d_variableQueue.top();
    d_variableQueue.pop();
    d_variableQueuePositions[var.index()] = variable_queue::point_iterator();

    if (d_trail.value(var).isNull()) {
      // Decide !var first
      out(Literal(var, true));
      return;
    }
  }
}


void BCPEngine::unsetVariables(const std::vector<Variable>& vars) {
  // Just add the Boolean variables to the queue
  for (unsigned i = 0; i < vars.size(); ++ i) {
    enqueue(vars[i]);
  }
}
