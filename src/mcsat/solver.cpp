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
, d_cnfStream(searchContext, &d_variableDatabase)
, d_cnfStreamListener(*this)
, d_trail(d_problemClauses, searchContext)
, d_rule_InputClause(d_problemClauses, d_trail)
{
  d_cnfStream.addOutputListener(&d_cnfStreamListener);

  // Add some engines
  addPlugin("CVC4::mcsat::BCPEngine");
}

Solver::~Solver() {
  for (unsigned i = 0; i < d_plugins.size(); ++ i) {
    delete d_plugins[i];
  }
}

void Solver::addAssertion(TNode assertion, bool process) {
  VariableDatabase::SetCurrent scoped(&d_variableDatabase);
  Debug("mcsat::solver") << "Solver::addAssertion(" << assertion << ")" << endl; 
  d_cnfStream.convert(theory::Rewriter::rewrite(assertion), false);
  if (process) {
    processNewClauses();
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
    // Propagate
    propagate();    
  }
  
  // Done with the clauses, clear
  d_newClauses.clear();
}

void Solver::propagate() {
  SolverTrail::PropagationToken out(d_trail);
  for (unsigned i = 0; d_trail.consistent() && i < d_plugins.size(); ++ i) {
    d_plugins[i]->propagate(out);
  }
}

bool Solver::check() {
  
  // Search while not all variables assigned 
  while (true) {

    // Process any new clauses 
    processNewClauses();
    
    // Propagate 
    propagate();
    
    // If no new clauses, we can do a decision
    if (d_newClauses.size() == 0) {
      Literal lit;
      SolverTrail::DecisionToken out(d_trail);
      out(lit);
    }
  }

  return true;
}

void Solver::addPlugin(std::string plugin) {
  d_plugins.push_back(SolverPluginFactory::create(plugin, d_trail));
}
