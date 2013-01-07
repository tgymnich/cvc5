#pragma once

#include "cvc4_private.h"

#include "expr/node.h"
#include "context/cdlist.h"
#include "context/cdinsert_hashmap.h"

#include "mcsat/variable/variable.h"
#include "mcsat/variable/variable_db.h"
#include "mcsat/clause/literal.h"

#include <ext/hash_map>

namespace CVC4 {
namespace mcsat {

class CnfOutputListener {
public:

  virtual ~CnfOutputListener() {}

  /** Notification of a new clause */
  virtual void newClause(Literals& clause) = 0;
};

class CnfStream {

public:

  /** Cache of what nodes have been registered to a literal. */
  typedef context::CDInsertHashMap<Literal, TNode, LiteralHashFunction> LiteralToNodeMap;

  /** Cache of what literals have been registered to a node. */
  typedef context::CDInsertHashMap<Node, Literal, NodeHashFunction> NodeToLiteralMap;

  /**
   * Constructs a CnfStream that sends constructs an equi-satisfiable
   * set of clauses and sends them to the given sat solver. All translation
   * info is kept with accordance to the given context object.
   */
  CnfStream(context::Context* cnfContext, VariableDatabase* variableDatabase);

  /**
   * Destructs a CnfStream.
   */
  virtual ~CnfStream() {
  }

  /**
   * Add a listener to this CNF transform.
   */
  void addOutputListener(CnfOutputListener* cnfListener) {
    d_outputNotifyList.push_back(cnfListener);
  }

  /**
   * Converts the formula to CNF.
   * @param node node to convert and assert
   * @param negated whether we are asserting the node negated
   */
  virtual void convert(TNode node, bool negated) = 0;

  /**
   * Get the node that is represented by the given SatLiteral.
   * @param literal the literal
   * @return the actual node
   */
  TNode getNode(Literal literal) const;

  /**
   * Returns true iff the node has an assigned literal (it might not be translated).
   * @param node the node
   */
  bool hasLiteral(TNode node) const;

  /**
   * Returns the literal that represents the given node in the SAT CNF
   * representation.
   */
  Literal getLiteral(TNode node) const;

private:

  /** Who gets the output of the CNF */
  std::vector<CnfOutputListener*> d_outputNotifyList;

  /** Map from nodes to literals */
  NodeToLiteralMap d_nodeToLiteralMap;

  /** Map from literals to nodes */
  LiteralToNodeMap d_literalToNodeMap;

  /** Variable database we're using */
  VariableDatabase* d_variableDatabase;

protected:
  
  /**
   * Output the clause.
   */
  void outputClause(Literals& clause);

  /**
   * Output the unit clause.
   */
  void outputClause(Literal a);

  /**
   * Output the binary clause.
   */
  void outputClause(Literal a, Literal b);

  /**
   * Ouput the ternary clause.
   */
  void outputClause(Literal a, Literal b, Literal c);

  /**
   * Constructs a new literal for an atom and returns it.
   */
  Literal convertAtom(TNode node);

  /** Create a new literal */
  Literal newLiteral(TNode node);


};/* class CnfStream */

}/* mcsat namespace */
}/* CVC4 namespace */
