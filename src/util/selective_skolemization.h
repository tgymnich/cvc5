/*********************                                                        */
/*! \file selective_skolemization.h
 ** \verbatim
 ** Original author: Dejan Jovanovic
 ** Major contributors: Kshitij Bansal, Morgan Deters
 ** Minor contributors (to current version): Andrew Reynolds, Clark Barrett
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief Removal of selected terms with skolems.
 **/

#include "cvc4_private.h"

#pragma once

#include <vector>
#include <string>

#include "expr/node.h"
#include "context/context.h"
#include "context/cdhashmap.h"

namespace CVC4 {

class Skolemizer {

public:
  
  /** Does this term requre skolemization */
  virtual bool skolemize(TNode current, TNode parent) const = 0; 
  
  /** Does this term requre skolemization */
  virtual Node skolemize(TNode current) = 0; 
};
  
class SkolemizationRunner {
  
  typedef context::CDHashMap<Node, Node, NodeHashFunction> SkolemizationCache;
  
  /** Map from terms to the skolem variables that replace them */
  SkolemizationCache d_skolemizationCache;

  /** The skolemizer */
  Skolemizer& d_skolemizer;
  
  Node run(TNode current, TNode parent, std::vector<Node>& additionalAssertions);
  
public:

  SkolemizationRunner(context::UserContext* u, Skolemizer& skolemizer) 
  : d_skolemizationCache(u)
  , d_skolemizer(skolemizer)
  {}

  /**
   * Removes the terms by introducing skolem variables. All
   * additional assertions are pushed into assertions.
   */
  void run(std::vector<Node>& assertions);

};/* class RemoveTTE */

}/* CVC4 namespace */
