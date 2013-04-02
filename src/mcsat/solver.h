#pragma once

#include "cvc4_private.h"

#include "expr/node.h"
#include "context/cdo.h"
#include "context/cdlist.h"
#include "util/statistics_registry.h"

#include "mcsat/solver_trail.h"
#include "mcsat/clause/clause_db.h"
#include "mcsat/plugin/solver_plugin.h"
#include "mcsat/rules/resolution_rule.h"

namespace CVC4 {
namespace mcsat {

/** Statistics for the bas Solver */
struct SolverStats {
  /** Number of conflicts */
  IntStat conflicts;
  /** Number of decisions */
  IntStat decisions;
  /** Number of restarts */
  IntStat restarts;
  
  SolverStats() 
  : conflicts("mcsat::solver::conflicts", 0)
  , decisions("mcsat::solver::decisions", 0)
  , restarts("mcsat::solver::restarts", 0)  
  {
    StatisticsRegistry::registerStat(&conflicts);  
    StatisticsRegistry::registerStat(&decisions);  
    StatisticsRegistry::registerStat(&restarts);  
  }
  
  ~SolverStats() {
    StatisticsRegistry::unregisterStat(&conflicts);  
    StatisticsRegistry::unregisterStat(&decisions);  
    StatisticsRegistry::unregisterStat(&restarts);  
  }
};

  
class Solver {

public:

  /** Construct the solver */
  Solver(context::UserContext* userContext, context::Context* searchContext);

  /** Destructor */
  virtual ~Solver();

  /** 
   * Assert the formula to the solver. If process is false the assertion will be
   * processed only after the call to check().
   * @param assertion the assertion
   * @param process true if to be processed as soon as possible 
   */
  void addAssertion(TNode assertion, bool process);

  /** Check if the current set of assertions is satisfiable */
  bool check();

  /** Add a plugin to the trail */
  void addPlugin(std::string plugin);

private:

  /** Solver statistics */
  SolverStats d_stats;
  
  /** The variable database of the solver */
  VariableDatabase d_variableDatabase;

  /** Farm for all the clauses */
  ClauseFarm d_clauseFarm;

  /** The clause database for input clauses */
  ClauseDatabase& d_clauseDatabase;

  /** Main solver trail */
  SolverTrail d_trail;

  /** Rewritten Assertions */
  std::vector<Node> d_assertions;

  /** Resolution rule for conflict analysis */
  rules::BooleanResolutionRule d_rule_Resolution;

  /** Clauses learnt from conflicts */
  std::vector<CRef_Strong> d_learntClauses;

  /** All the plugins */
  std::vector<SolverPlugin*> d_plugins;

  /** Notification dispatch */
  NotificationDispatch d_notifyDispatch;
  
  /** Features dispatch (propagate, decide) */
  FeatureDispatch d_featuresDispatch;

  /** The requests of the plugins */
  std::vector<SolverPluginRequest*> d_pluginRequests;
  
  /** Process any requests from solvers */
  void processRequests();

  /** Perform propagation */
  void propagate(SolverTrail::PropagationToken::Mode mode);

  /** Analyze all the conflicts in the trail */
  void analyzeConflicts();

  /** Has there been any requests */
  bool d_request;
  
  /** Has there been a backtrack request */
  bool d_backtrackRequested;

  /** The level of the backtrack request */
  size_t d_backtrackLevel;

  /** The clauses to be processed on backtrack */
  std::set<CRef> d_backtrackClauses;

  /** Will perform a backtrack in order to propagate/decide clause cRef at next appropriate time */
  void requestBacktrack(unsigned level, CRef cRef);
  
  /** Has a restart been requested */
  bool d_restartRequested;
  
  /** Request a search restart */
  void requestRestart();
  
  friend class SolverPluginRequest;

  /** Scores of learnt clauses */
  std::hash_map<CRef, double, CRefHashFunction> d_learntClausesScore;

  /** Maximal score */
  double d_learntClausesScoreMax;

  /** Increase per bump */
  double d_learntClausesScoreIncrease;

  /** Increase the clause heuristic */
  void bumpClause(CRef cRef);

  /** Heuristically remove some learnt clauses */
  void shrinkLearnts();

  /**
   * Class responsible for traversing the input formulas and registering variables
   * with the variable database.
   */
  class VariableRegister {

    /** Variable database we're using */
    VariableDatabase& d_varDb;

    /** Set of already visited nodes */
    std::hash_set<TNode, TNodeHashFunction> d_visited;

    /** The list of all variables */
    std::vector<Variable> d_variables;
    
  public:

    typedef void return_type;

    VariableRegister(VariableDatabase& varDb)
    : d_varDb(varDb) {}

    bool alreadyVisited(TNode current, TNode parent) const {
      return d_visited.find(current) != d_visited.end();
    }

    void visit(TNode current, TNode parent) {
      if (current.isVar()) {
	Variable var = d_varDb.getVariable(current);
	d_variables.push_back(var);
      }
      d_visited.insert(current);      
    }

    void start(TNode node) {}
    void done(TNode node) {}
    
    const std::vector<Variable>& getVariables() const {
      return d_variables;
    }
  } d_variableRegister;

};

}
}




