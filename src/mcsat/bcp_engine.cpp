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

BCPEngine::BCPEngine(const SolverTrail& trail, SolverPluginRequest& request)
: SolverPlugin(trail, request)
, d_newClauseNotify(*this)
, d_trailHead(d_trail.getSearchContext())
{
  d_trail.addNewClauseListener(&d_newClauseNotify);
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
      return d_trail.trailIndex(l1.getVariable()) > d_trail.trailIndex(l2.getVariable());
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
    d_delayedPropagations.push_back(cRef);
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
      
      Literal toPropagate;
      
      Variable var_value = d_trail.value(var);

      if (var_value == c_TRUE) {
        // Look for ~var in clauses
	toPropagate = Literal(var, true);
      } else if (var_value == c_FALSE) {
        // Look for var in clause
	toPropagate = Literal(var, false);
      } else {
        // Everything in the trail must have a value
        Unreachable();
      }
      
      Debug("mcsat::bcp") << "BCPEngine::propagate(): propagating " << ~toPropagate << std::endl;

      // Get the watchlist of the literal to propagate
      WatchListManager::remove_iterator w = d_watchManager.begin(toPropagate);
      
      // Propagate through the watchlist
      while (d_trail.consistent() && !w.done()) {
	// Get the watched clause
	CRef cRef = *w;
	Clause& clause = cRef.getClause();
	
        // Put the propagation lister at position 1
	if (clause[0] == toPropagate) {
	  clause.swapLiterals(0, 1);
	}
        
        // If 0th literal is true, the clause is already satisfied.
        if (d_trail.value(clause[0]) == c_TRUE) {
	  // Keep this watch and go to the next one 
	  w.next_and_keep();
	  continue;
	}

	// Try to find a new watch
        for (unsigned j = 2; j < clause.size(); j++) {
	  if (d_trail.value(clause[j]) != c_FALSE) {
	    // Put the new watch in place
	    clause.swapLiterals(1, j);
	    d_watchManager.add(~clause[1], cRef);
	    // Don't keep this watch, and go to the next one
	    w.next_and_remove();
	    continue;
	  }
	}

	// We did not find a watch so clause[0] is propagated to be true
        out(clause[0], cRef);
      }
    }
  }

  /** Update the CD trail head */
  d_trailHead = i;
}

void BCPEngine::decide(SolverTrail::DecisionToken& out) {

}
