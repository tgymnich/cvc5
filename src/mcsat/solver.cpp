#include "solver.h"

#include "theory/rewriter.h"
#include "plugin/solver_plugin_factory.h"
#include "rules/proof_rule.h"

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

void Solver::CnfListener::newClause(LiteralVector& clause) {
  outerClass().addClause(clause);
}

Solver::Solver(context::UserContext* userContext, context::Context* searchContext) 
: d_variableDatabase(searchContext)
, d_clauseFarm(searchContext)
, d_problemClauses(d_clauseFarm.newClauseDB("problem_clauses"))
, d_auxilaryClauses(d_clauseFarm.newClauseDB("conflict_analysis_clauses"))
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

void Solver::addClause(LiteralVector& clause) {
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
  Debug("mcsat::solver") << "propagate(" << mode << ")" << std::endl;

  bool propagationDone = false;
  while (d_trail.consistent() && !propagationDone) {
    // Token for the plugins to propagate at
    SolverTrail::PropagationToken propagationOut(d_trail, mode);
    // Let all plugins propagate
    for (unsigned i = 0; d_trail.consistent() && i < d_plugins.size(); ++ i) {
      Debug("mcsat::solver") << "propagate(" << mode << "): propagating with " << *d_plugins[i] << std::endl;
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

      Debug("mcsat::solver::search") << "Conflict" << std::endl;

      // If the conflict is at level 0, we're done
      if (d_trail.decisionLevel() == 0) {
        return false;
      }

      // Analyze the conflict
      analyzeConflicts();

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

/**
 * Returns the level at which the clause is in conflict, or negative if it's not in conflict.
 * The level of the conflict is the level at which the clause propagates a literal that is assigned
 * the opposite polarity.
 */
unsigned getConflictLevel(Clause& clause, SolverTrail& trail) {

  Assert(clause.size() > 1);

  unsigned trailIndex1 = 0; // Index of the literal with the top trail index
  unsigned trailIndex2 = 0; // Index of the literal with the second top trail index

  unsigned level1 = 0; // Level of the bad propagation

  for (unsigned i = 0; i < clause.size(); ++ i) {
    const Literal& literal = clause[i];
    Variable variable = literal.getVariable();

    Assert(trail.value(literal) == trail.c_FALSE);

    unsigned trailIndex = trail.trailIndex(variable);

    if (trailIndex > trailIndex1) {
      trailIndex2 = trailIndex1;
      trailIndex1 = trailIndex;
      level1 = trail.decisionLevel(variable);
    } else if (trailIndex > trailIndex2) {
      trailIndex2 = trailIndex;
    }
  }

  return level1;
}

void Solver::analyzeConflicts() {

  // Conflicts to resolve
  std::vector<SolverTrail::InconsistentPropagation> conflicts;
  d_trail.getInconsistentPropagations(conflicts);

  // Output resolvents
  std::vector<CRef> resolvents;

  Assert(conflicts.size() > 0);
  Debug("mcsat::solver::analyze") << "Solver::analyzeConflicts()" << std::endl;

  for (unsigned c = 0; c < conflicts.size(); ++ c) {

    // Get the current conflict
    SolverTrail::InconsistentPropagation& conflictPropagation = conflicts[c];

    // Clause in conflict
    Clause& conflictingClause = conflictPropagation.reason.getClause();
    Debug("mcsat::solver::analyze") << "Solver::analyzeConflicts(): analyzing " << conflictingClause << std::endl;

    // The level at which the clause is in conflict
    unsigned conflictLevel = getConflictLevel(conflictingClause, d_trail);
    Debug("mcsat::solver::analyze") << "Solver::analyzeConflicts(): in conflict at level " << conflictLevel << std::endl;

    // The Boolean resolution rule
    rules::BooleanResolutionRule resolution(d_auxilaryClauses, conflictPropagation.reason);

    // Set of literals to resolve at the top level
    VariableHashSet toResolve;
    // Count of literals we have in the resolvent at the conflict level
    unsigned literalsAtConflictLevel = 0;

    for (unsigned i = 0; i < conflictingClause.size(); ++ i) {
      const Literal& literal = conflictingClause[i];
      // We resolve literals at the conflict level
      if (d_trail.decisionLevel(literal.getVariable()) == conflictLevel) {
        literalsAtConflictLevel ++;
        if (d_trail.hasReason(~literal)) {
          // Since the negation has a reason, we'll resolve it
          toResolve.insert(literal.getVariable());
        }
      }
    }

    // Resolve out all literals at the top level
    Assert(d_trail.size(conflictLevel) >= 1);
    unsigned trailIndex = d_trail.size(conflictLevel) - 1;
    // While there is more than one literal at current conflict level (UIP)
    while (literalsAtConflictLevel > 1) {

      // Find the next literal to resolve
      while (toResolve.find(d_trail[trailIndex].var) == toResolve.end()) {
        -- trailIndex;
      }

      // The variable we're resolving
      Variable varToResolve = d_trail[trailIndex].var;
      Variable varToResolveValue = d_trail.value(varToResolve);
      Assert(!varToResolve.isNull());

      // If the variable is false, take the negated literal
      Literal literalToResolve(varToResolve, varToResolveValue == d_trail.c_FALSE);

      // Resolve the literal (propagations should always have first literal propagating)
      resolution.resolve(d_trail.reason(literalToResolve), 0);

    }

    // Finish the resolution
    CRef resolvent = resolution.finish();
    Debug("mcsat::solver::analyze") << "Solver::analyzeConflicts(): resolvent: " << resolvent << std::endl;
    resolvents.push_back(resolvent);
  }

  //

  exit(0);
}
