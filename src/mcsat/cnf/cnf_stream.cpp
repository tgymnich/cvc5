#include "cnf_stream.h"

using namespace std;

using namespace CVC4;
using namespace mcsat;

CnfStream::CnfStream(context::Context* cnfContext, VariableDatabase* variableDatabase)
: d_nodeToLiteralMap(cnfContext)
, d_literalToNodeMap(cnfContext)
, d_variableDatabase(variableDatabase)
{
}

void CnfStream::outputClause(Literals& c) {
  Debug("mcsat::cnf") << "CNF Output " << c << endl;
  for (unsigned i = 0; i < d_outputNotifyList.size(); ++ i) {
    d_outputNotifyList[i]->newClause(c);
  }
}

void CnfStream::outputClause(Literal a) {
  Literals clause(1);
  clause[0] = a;
  outputClause(clause);
}

void CnfStream::outputClause(Literal a, Literal b) {
  Literals clause(2);
  clause[0] = a;
  clause[1] = b;
  outputClause(clause);
}

void CnfStream::outputClause(Literal a, Literal b, Literal c) {
  Literals clause(3);
  clause[0] = a;
  clause[1] = b;
  clause[2] = c;
  outputClause(clause);
}

bool CnfStream::hasLiteral(TNode n) const {
  NodeToLiteralMap::const_iterator find = d_nodeToLiteralMap.find(n);
  return find != d_nodeToLiteralMap.end();
}

TNode CnfStream::getNode(Literal literal) const {
  Debug("mcsat::cnf") << "getNode(" << literal << ")" << endl;
  Debug("mcsat::cnf") << "getNode(" << literal << ") => " << d_literalToNodeMap[literal] << endl;
  LiteralToNodeMap::const_iterator find = d_literalToNodeMap.find(literal);
  if (find == d_literalToNodeMap.end()) {
    return find->second;
  } else {
    return TNode::null();
  }
}

Literal CnfStream::convertAtom(TNode node) {
  Debug("mcsat::cnf") << "convertAtom(" << node << ")" << endl;

  Assert(!hasLiteral(node), "atom already mapped!");

  // Make a new literal (variables are not considered theory literals)
  Literal lit = newLiteral(node);

  // Return the resulting literal
  return lit;
}

Literal CnfStream::newLiteral(TNode node) {
  Debug("mcsat::cnf") << "newLiteral(" << node << ")" << endl;
  
  Assert(node.getKind() != kind::NOT);

  // Get the literal for this node
  Literal lit;
  if (!hasLiteral(node)) {
    // Create a new variable
    Variable variable = d_variableDatabase->getVariable(node);
    // Make the literal
    lit = Literal(variable, false);
    // Record the literal
    d_nodeToLiteralMap.insert(node, lit);
    d_nodeToLiteralMap.insert(node.notNode(), ~lit);
  } else {
    lit = getLiteral(node);
  }

  return lit;
}

Literal CnfStream::getLiteral(TNode node) const {
  Assert(!node.isNull(), "CnfStream: can't getLiteral() of null node");
  Assert(d_nodeToLiteralMap.contains(node), "Literal not in the CNF Cache: %s\n", node.toString().c_str());
  Literal literal = d_nodeToLiteralMap[node];
  Debug("mcsat::cnf") << "CnfStream::getLiteral(" << node << ") => " << literal << std::endl;
  return literal;
}

