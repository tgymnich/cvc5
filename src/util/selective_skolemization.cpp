/*********************                                                        */
/*! \file selective_skolemization.cpp
 ** \verbatim
 ** Original author: Dejan Jovanovic
 ** Major contributors: Andrew Reynolds, Morgan Deters
 ** Minor contributors (to current version): Clark Barrett
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief Replace selected terms with skolem variables 
 **
 ** Removal of term ITEs.
 **/

#include <vector>

#include "util/selective_skolemization.h"
#include "theory/rewriter.h"

using namespace CVC4;
using namespace std;

void SkolemizationRunner::run(std::vector<Node>& output)
{
  Debug("skolemization") << "SkolemizationRunner::run(): start" << endl;
  for (unsigned i = 0; i < output.size(); ++ i) {
    output[i] = run(output[i], output[i], output);
  }
  Debug("skolemization") << "SkolemizationRunner::run(): end" << endl;
}

Node SkolemizationRunner::run(TNode node, TNode parent, std::vector<Node>& output) {
  // Current node
  Debug("skolemization") << "SkolemizationRunner::run(" << node << ")" << endl;

  bool definition = false;
  
  // The result may be cached already
  NodeManager *nodeManager = NodeManager::currentNM();
  SkolemizationCache::iterator i = d_skolemizationCache.find(node);
  if(i != d_skolemizationCache.end()) {
    Node cachedRewrite = (*i).second;
    
    // We did this before
    if (cachedRewrite.isNull()) {
      Debug("skolemization") << "SkolemizationRunner::run(" << node << "): cached " << node << endl;
      return node;
    }
    
    // Is this a definition we introduced
    if (parent.getKind() == kind::EQUAL && parent[0] == cachedRewrite && parent[1] == node) {
      definition = true;
    } else {
      Debug("skolemization") << "SkolemizationRunner::run(" << node << "): cached " << cachedRewrite << endl;
      return cachedRewrite;
    }
  }

  // Do we replace 
  if(!definition && d_skolemizer.skolemize(node, parent)) {

    // Get the skolemization info 
    Node skolem = d_skolemizer.skolemize(node);
  
    // Attach the skolem
    d_skolemizationCache[node] = skolem;
    d_skolemizationCache[skolem] = skolem;

    Debug("skolemization") << "SkolemizationRunner::run(" << node << "): skolemizing: " << node << " -> " << skolem << endl;
  
    // Remember the definition 
    output.push_back(skolem.eqNode(node));

    // The representation is now the skolem
    return skolem;
  } else {

    Debug("skolemization") << "SkolemizationRunner::run(" << node << "): skolemizing children" << endl;

    // Don't change it
    vector<Node> newChildren;
    bool somethingChanged = false;
    if(node.getMetaKind() == kind::metakind::PARAMETERIZED) {
      newChildren.push_back(node.getOperator());
    }
    // Remove the terms from the children 
    for(TNode::const_iterator it = node.begin(), end = node.end(); it != end; ++it) {
      Node newChild = run(*it, node, output);
      somethingChanged |= (newChild != *it);
      newChildren.push_back(newChild);
    }
    // If changes, we rewrite
    if(somethingChanged) {
      Node newNode = nodeManager->mkNode(node.getKind(), newChildren);
      d_skolemizationCache[node] = newNode;
      return newNode;
    } else {
      d_skolemizationCache[node] = Node::null();
      return node;
    }
  } 
}

