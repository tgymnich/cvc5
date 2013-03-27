#include "mcsat/cnf/cnf_plugin.h"

using namespace CVC4;
using namespace mcsat;

CNFPlugin::CNFPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request)
: SolverPlugin(database, trail, request)
, d_cnfStreamListener(*this)
, d_inputClauseRule(database, trail)
{
  // We want to know about any assertions
  addNotification(NOTIFY_ASSERTION);

  // We get the clauses from the cnf converter
  d_cnfStream.addOutputListener(&d_cnfStreamListener);
}

std::string CNFPlugin::toString() const  {
  return "CNF Conversion";
}

void CNFPlugin::CnfListener::newClause(LiteralVector& clause) {
  CRef cRef = d_cnfPlugin.d_inputClauseRule.apply(clause);
  d_cnfPlugin.d_convertedClauses.push_back(cRef);
}

void CNFPlugin::notifyAssertion(TNode assertion) {
  // Convert the assertion
  d_cnfStream.convert(assertion, false);
}
