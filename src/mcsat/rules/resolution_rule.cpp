#include "mcsat/rules/resolution_rule.h"

using namespace CVC4;
using namespace mcsat;
using namespace rules;

BooleanResolutionRule::BooleanResolutionRule(ClauseDatabase& clauseDB)
: ProofRule("mcsat::resolution_rule", clauseDB)
{
}

void BooleanResolutionRule::start(CRef initialClause) {
  Debug("mcsat::resolution_rule") << "BooleanResolutionRule(): starting with " << initialClause << std::endl;

  Assert(d_literals.size() == 0);

  Clause& clause = initialClause.getClause();
  for (unsigned i = 0; i < clause.size(); ++ i) {
    d_literals.insert(clause[i]);
  }
}

void BooleanResolutionRule::resolve(CRef cRef, unsigned literalIndex) {
  Debug("mcsat::resolution_rule") << "BooleanResolutionRule(): resolving " << d_literals << " with " << cRef << std::endl;
  Clause& toResolve = cRef.getClause();
  Assert(literalIndex < toResolve.size());

  // Add all the literals and remove the one we're resolving
  for (unsigned i = 0; i < toResolve.size(); ++ i) {
    Literal l = toResolve[i];
    if (i == literalIndex) {
      Literal not_l = ~l;
      LiteralHashSet::iterator find = d_literals.find(not_l);
      Assert(find != d_literals.end());
      d_literals.erase(find);
    } else {
      Assert(d_literals.find(~l) == d_literals.end());
      d_literals.insert(l);
    }
  }

  // Make sure that empty clause is false
  if (d_literals.size() == 0) {
    Node falseNode = NodeManager::currentNM()->mkConst<bool>(false);
    Variable falseVar = VariableDatabase::getCurrentDB()->getVariable(falseNode);
    d_literals.insert(Literal(falseVar, false));
  }

  Debug("mcsat::resolution_rule") << "BooleanResolutionRule(): got " << d_literals << std::endl;
}

CRef BooleanResolutionRule::finish() {
  CRef result = commit(d_literals);
  d_literals.clear();
  return result;
}

