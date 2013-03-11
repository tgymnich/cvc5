#include "proof_rule.h"

using namespace CVC4;
using namespace CVC4::mcsat;
using namespace CVC4::mcsat::rules;

using namespace std;

CRef ProofRule::commit(LiteralVector& literals) {
  Debug("mcsat::rules") << "ProofRule<" << d_name << ">::commit(" << literals << ")" << endl;
  ++ d_applications;
  return d_clauseDB.newClause(literals, d_id);
}

CRef ProofRule::commit(LiteralSet& literals) {
  Debug("mcsat::rules") << "ProofRule<" << d_name << ">::commit(" << literals << ")" << endl;
  ++ d_applications;
  LiteralVector literalVector(literals.begin(), literals.end());
  return d_clauseDB.newClause(literalVector, d_id);
}

CRef ProofRule::commit(LiteralHashSet& literals) {
  Debug("mcsat::rules") << "ProofRule<" << d_name << ">::commit(" << literals << ")" << endl;
  ++ d_applications;
  LiteralVector literalVector(literals.begin(), literals.end());
  return d_clauseDB.newClause(literalVector, d_id);
}

ProofRule::ProofRule(string name, ClauseDatabase& clauseDb)
: d_name(name)
, d_applications(name, 0)
, d_clauseDB(clauseDb)
, d_id(clauseDb.registerRule())
{
  StatisticsRegistry::registerStat(&d_applications);  
}

ProofRule::~ProofRule() {
  StatisticsRegistry::unregisterStat(&d_applications);
}
