#include "proof_rule.h"

using namespace CVC4;
using namespace CVC4::mcsat;
using namespace CVC4::mcsat::rules;

using namespace std;

CRef_Strong ProofRule::commit(Literals& literals) {
  Debug("mcsat::rules") << "ProofRule<" << d_name << ">::commit(" << literals << ")" << endl;
  ++ d_applications;
  return d_clauseDB.newClause(literals, d_id);
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

CRef_Strong InputClauseRule::apply(Literals& literals) {

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
      return CRef_Strong::null;
      break;
    }
      
    // Check if the literal is assigned already 
    Variable value = d_trail.value(literals[i]);
      
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
    previous = i;
  }
    
  // Resize to clause size
  literals.resize(previous + 1);
    
  if (literals.size() == 0) {
    // Push back the false literal 
  } 
    
  // Make the clause
  return commit(literals);
}
