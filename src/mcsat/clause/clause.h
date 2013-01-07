#pragma once

#include <vector>
#include <iostream>

#include "cvc4_private.h"

#include "mcsat/clause/literal.h"

namespace CVC4 {
namespace mcsat {

class Clause {

  /** Size of the clause */
  size_t d_size     : 32;
  /** Reference count */
  size_t d_refCount : 16;
  /** Rule that produced this clause */
  size_t d_ruleId   : 16;

  typedef Literal_Strong literal_type;

  /** The literals */
  literal_type d_literals[0];

  friend class ClauseDatabase;

  /** Construct the clause from given literals */
  Clause(const std::vector<Literal>& literals, size_t ruleId);

  /** Copy construction */
  Clause(const Clause& clause);
  
  template<bool refCount>
  friend class ClauseRef;

  /** Increase the reference count */
  void incRefCount() { d_refCount ++; }

  /** Decrease the reference count */
  void decRefCount() { d_refCount ++; }

public:

  ~Clause();

  /** Number of literals in the clause */
  size_t size() const {
    return d_size;
  }

  /** Get the literal at position i */
  const Literal& operator[] (size_t i) const {
    return reinterpret_cast<const Literal&>(d_literals[i]);
  }

  /** Swap the literals at given positions */
  void swapLiterals(size_t i, size_t j) {
    // Swap is overloaded so that no references are counted
    std::swap(d_literals[i], d_literals[j]);
  }

  /** Is this clause in use */
  bool inUse() const {
    return d_refCount > 0; 
  }

  template <class Compare>
  void sort(Compare& cmp);

  /** Simple stream representation of the clause */
  void toStream(std::ostream& out) const;
};

/** Output operator for clauses */
inline std::ostream& operator << (std::ostream& out, const Clause& clause) {
  clause.toStream(out);
  return out;
}

// Sorting as Literals since we only shuffle them around
template<class Compare>
inline void Clause::sort(Compare& cmp) {
  Literal* begin = reinterpret_cast<Literal*>(d_literals);
  Literal* end = reinterpret_cast<Literal*>(d_literals) + size();
  std::sort(begin, end, cmp);
}

} /* Namespace mcsat */
} /* namespace CVC4 */

