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
  CRef_Strong commit(Literals& literals);
public:
  /** Virtual destructor */
  virtual ~ProofRule();
};

/**
 * Rule for adding input clauses preprocessed wrt the current trail.
 */
class InputClauseRule : public ProofRule {
  const SolverTrail& d_trail;
public:
  InputClauseRule(ClauseDatabase& clauseDB, const SolverTrail& trail)
  : ProofRule("mcsat::input_clause_rule", clauseDB)
  , d_trail(trail) {}

  /** Simplify and add the clause to the database */
  CRef_Strong apply(Literals& literals);
};

}
}
}



