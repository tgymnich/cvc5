#include "solver.h"

#include "theory/rewriter.h"
#include "plugin/solver_plugin_factory.h"

#include <algorithm>

using namespace std;

using namespace CVC4;
using namespace CVC4::mcsat;

template<typename T>
class ScopedValue {
  T& d_toWatch;
  T d_oldValue;
public:
  ScopedValue(T& toWatch, T newValue)
  : d_toWatch(toWatch)
  , d_oldValue(toWatch)
  {
    toWatch = newValue;
  }
  ~ScopedValue() {
    d_toWatch = d_oldValue;
  } 
};

void Solver::CnfListener::newClause(Literals& clause) {
  outerClass().addClause(clause);
}

Solver::Solver(context::UserContext* userContext, context::Context* searchContext) 
: d_variableDatabase(searchContext)
, d_clauseFarm(searchContext)
, d_problemClauses(d_clauseFarm.newClauseDB("problem_clauses"))
, d_cnfStream(searchContext)
, d_cnfStreamListener(*this)
, d_trail(d_problemClauses, searchContext)
, d_rule_InputClause(d_problemClauses, d_trail)
, d_backtrackRequested(false)
, d_backtrackLevel(0)
{
  d_cnfStream.addOutputListener(&d_cnfStreamListener);

  // Add some engines
  addPlugin("CVC4::mcsat::BCPEngine");
}

Solver::~Solver() {
  for (unsigned i = 0; i < d_plugins.size(); ++ i) {
    delete d_plugins[i];
    delete d_pluginRequests[i];
  }
}

void Solver::addAssertion(TNode assertion, bool process) {
  VariableDatabase::SetCurrent scoped(&d_variableDatabase);
  Debug("mcsat::solver") << "Solver::addAssertion(" << assertion << ")" << endl; 

  // Convert to CNF
  d_cnfStream.convert(theory::Rewriter::rewrite(assertion), false);

  // Process the clause immediately if requested
  if (process) {
    // Add the new clauses
    processNewClauses();
    // Propagate the added clauses
    propagate(SolverTrail::PropagationToken::PROPAGATION_INIT);
  }
}

void Solver::addClause(Literals& clause) {
  // Add to the list of new clauses to process 
  d_newClauses.push_back(clause);
}

void Solver::processNewClauses() {
 
  // Add all the clauses
  for (unsigned clause_i = 0; clause_i < d_newClauses.size(); ++ clause_i) {
    // Apply the input clause rule
    d_rule_InputClause.apply(d_newClauses[clause_i]);
  }
  
  // Done with the clauses, clear
  d_newClauses.clear();
}

void Solver::propagate(SolverTrail::PropagationToken::Mode mode) {
  bool propagationDone = false;
  while (d_trail.consistent() && !propagationDone) {
    // Token for the plugins to propagate at
    SolverTrail::PropagationToken propagationOut(d_trail, mode);
    // Let all plugins propagate
    for (unsigned i = 0; d_trail.consistent() && i < d_plugins.size(); ++ i) {
      d_plugins[i]->propagate(propagationOut);
    }
    // If the token wasn't used, we're done
    propagationDone = !propagationOut.used();
  }
}

bool Solver::check() {
  
  // Search while not all variables assigned 
  while (true) {

    // Process any new clauses 
    processNewClauses();
    
    // Normal propagate
    propagate(SolverTrail::PropagationToken::PROPAGATION_NORMAL);

    // If inconsistent, perform conflict analysis
    if (!d_trail.consistent()) {

      // If the conflict is at level 0, we're done
      if (d_trail.decisionLevel() == 0) {
        return false;
      }

      // Analyze the conflict

      // Start over with propagation
      continue;
    }
    
    // If new clauses were added, we need to process them
    if (!d_newClauses.empty()) {
      continue;
    }

    // Clauses processed, propagators done, we're ready for a decision
    SolverTrail::DecisionToken decisionOut(d_trail);

    for (unsigned i = 0; !decisionOut.used() && i < d_plugins.size(); ++ i) {
      d_plugins[i]->decide(decisionOut);
    }

    // If no decisions were made we are done, do a completeness check
    if (!decisionOut.used()) {
      // Do complete propagation
      propagate(SolverTrail::PropagationToken::PROPAGATION_COMPLETE);
      // If no-one has anything to say, we're done
      break;
    }
  }

  return true;
}

void Solver::addPlugin(std::string plugin) {
  d_pluginRequests.push_back(new SolverPluginRequest(this));
  d_plugins.push_back(SolverPluginFactory::create(plugin, d_trail, *d_pluginRequests.back()));
}

void Solver::requestBacktrack(unsigned level, CRef cRef) {
  Assert(level < d_trail.decisionLevel(), "Don't try to fool backtracking to do your propagation");

  if (!d_backtrackRequested) {
    // First time request
    d_backtrackLevel = level;
  } else {
    if (d_backtrackLevel > level) {
      // Even lower backtrack
      d_backtrackLevel = level;
      d_backtrackClauses.clear();
    }
  }

  d_backtrackRequested = true;
  d_backtrackClauses.push_back(cRef);
}

