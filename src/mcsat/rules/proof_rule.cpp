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

CRef InputClauseRule::apply(LiteralVector& literals) {

  // Get the true/false constants
  Variable c_True = d_trail.getTrue();
  Variable c_False = d_trail.getFalse();

  // Sort the literals to check for duplicates and tautologies
  std::sort(literals.begin(), literals.end());
    
  int previous = -1;
  for (unsigned i = 0; i < literals.size(); ++ i) 
  {
    // Ignore duplicate literals 
    if (previous >= 0 && literals[i] == literals[previous]) {
      continue;
    }
      
    // Tautologies are ignored
    if (previous >= 0 && literals[i].isNegationOf(literals[previous])) {
      return CRef::null;
      break;
    }
      
    // Check if the literal is assigned already 
    Variable value = d_trail.value(literals[i]);
    if (!value.isNull()) {
      // This value has a meaning only if it's at 0 level
      if (d_trail.decisionLevel(literals[i].getVariable()) > 0) {
	value = Variable::null;
      }
    }
    
    if (value == c_True) {
      // Literal is true and hence the clause too
      return CRef_Strong::null;
      break;
    }
    if (value == c_False) {
      // Literal is false and can be ignored 
      continue;
    }
      
    // Move on
    literals[++previous] = literals[i];
  }
    
  // Resize to clause size
  literals.resize(previous + 1);
    
  if (literals.size() == 0) {
    Node falseNode = NodeManager::currentNM()->mkConst<bool>(false);
    Variable falseVar = VariableDatabase::getCurrentDB()->getVariable(falseNode);
    literals.push_back(Literal(falseVar, false));
  } 
    
  // Make the clause
  return commit(literals);
}


BooleanResolutionRule::BooleanResolutionRule(ClauseDatabase& clauseDB, CRef initialClause)
: ProofRule("mcsat::resolution_rule", clauseDB) {
  Debug("mcsat::resolution_rule") << "BooleanResolutionRule(): starting with " << initialClause << std::endl;
  Clause& clause = initialClause.getClause();
  for (unsigned i = 0; i < clause.size(); ++ i) {
    d_literals.insert(clause[i]);
  }
}

void BooleanResolutionRule::resolve(CRef cRef, unsigned literalIndex) {
  Debug("mcsat::resolution_rule") << "BooleanResolutionRule(): resolving " << d_literals << " with " << cRef << std::endl;
  Clause& toResolve = cRef.getClause();
  Assert(literalIndex < toResolve.size());
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
  Debug("mcsat::resolution_rule") << "BooleanResolutionRule(): got " << d_literals << std::endl;
}

CRef BooleanResolutionRule::finish() {
  return commit(d_literals);
}
