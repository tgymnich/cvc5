#pragma once

#include "cvc4_private.h"

#include <string>
#include <vector>

#include "mcsat/clause/clause_db.h"
#include "mcsat/solver_trail.h"

#include "util/statistics_registry.h"

namespace CVC4 {
namespace mcsat {
namespace rules {

/**
 * A proof rule encapsulates the procedural part of the deduction process. It
 * should be lightweight, created once, so that the compiler can optimize it's usage.
 * Proof rules are the only entities allowed to create clauses.
 */
class ProofRule {
private:
  /** Descriptive name of the proof rule */
  std::string d_name;
  /** Statistic for number of applications */
  IntStat d_applications;
  /** Clause database this rule is using */
  ClauseDatabase& d_clauseDB;
  /** Id of the rule with the clause database */
  size_t d_id;
protected:
  /** Construct the rule with the given name */
  ProofRule(std::string name, ClauseDatabase& clauseDB);
  /** Commit the result of the proof rule */
  CRef commit(LiteralVector& literals);
  /** Commit the result of the proof rule */
  CRef commit(LiteralSet& literals);
  /** Commit the result of the proof rule */
  CRef commit(LiteralHashSet& literals);
public:
  /** Virtual destructor */
  virtual ~ProofRule();
};

/**
 * Rule for adding input clauses preprocessed wrt the current trail. Can be applied
 * to many clauses.
 */
class InputClauseRule : public ProofRule {
  const SolverTrail& d_trail;
public:
  InputClauseRule(ClauseDatabase& clauseDB, const SolverTrail& trail)
  : ProofRule("mcsat::input_clause_rule", clauseDB)
  , d_trail(trail) {}

  /** Simplify and add the clause to the database */
  CRef apply(LiteralVector& literals);
};

/**
 * Boolean resolutino rule. To be used in sequence for one resolution proof.
 */
class BooleanResolutionRule : public ProofRule {

  /** The literals of the current clause */
  LiteralHashSet d_literals;

public:

  /** Create a new Boolean resolution starting from the given initial clause */
  BooleanResolutionRule(ClauseDatabase& clauseDB, CRef initialClause);

  /**
   * Resolve with given clause. Optionally, you can give the index of the literal to be resolved.
   */
  void resolve(CRef cRef, unsigned literalIndex);

  /** Finish the derivation */
  CRef finish();
};

}
}
}



