#pragma once

#include "cvc4_private.h"

#include <iostream>
#include <tr1/functional>
#include <boost/static_assert.hpp>

#include "mcsat/variable/variable.h"

namespace CVC4 {
namespace mcsat {

template <bool refCount>
class LiteralRef;

typedef LiteralRef<true> Literal_Strong;
typedef LiteralRef<false> Literal;
  
/** Literal encapsulates a variable and whether it is negated or not */
template <bool refCount>
class LiteralRef {

  friend void std::swap<>(LiteralRef& l1, LiteralRef& l2);

  /** The variable */
  VariableRef<refCount> d_variable;

  /** Is the variable negated */
  bool d_negated;
  
  friend class LiteralRef<!refCount>;

public:

  /** The null literal */
  static const LiteralRef null;

  /** Construct the literal given the variable and whether it is negated */
  LiteralRef(Variable var, bool negated)
  : d_variable(var)
  , d_negated(negated)
  {
  }

  /** Construct a null literal */
  LiteralRef()
  : d_variable(), d_negated(0) {}

  /** Copy constructor */
  LiteralRef(const LiteralRef& other)
  : d_variable(other.d_variable)
  , d_negated(other.d_negated)
  {}

  /** Copy constructor from other type of literal */
  LiteralRef(const LiteralRef<!refCount>& other)
  : d_variable(other.d_variable)
  , d_negated(other.d_negated)
  {}

  /** Assignment */
  LiteralRef& operator = (const LiteralRef& other) {
    if (this != &other) {
      d_variable = other.d_variable;
      d_negated = other.d_negated;
    }
    return *this;
  }

  /** Assignment from other type */
  LiteralRef& operator = (const LiteralRef<!refCount>& other) {
    d_variable = other.d_variable;
    d_negated = other.d_negated;
    return *this;
  }

  /** Return the variable of the literal */
  Variable getVariable() const { return d_variable; }

  /** Return true if the literal is negated */
  bool isNegated() const { return d_negated; }

  /** Return true if hte literal is the negation of the given literal */
  bool isNegationOf(const Literal& lit) const {
    if (d_variable != lit.d_variable) return false;
    return d_negated != lit.d_negated;
  }

  /** Negate this literal */
  void negate() { d_negated = !d_negated; }

  /** Return the nagation of this literal */
  Literal operator ~ () const {
    return Literal(d_variable, !d_negated);
  }

  /** 0-based index of the literal */
  size_t index() const {
    return (d_variable.index() << 1) | (d_negated ? 1 : 0);
  }

  /** Compare with some other literal */
  bool operator == (const Literal& other) const {
    return index() == other.index();
  }

  /** Compare with some other literal */
  bool operator != (const Literal& other) const {
    return index() != other.index();
  }

  /** Compare with some other literal */
  bool operator < (const Literal& other) const {
    return index() < other.index();
  }

  /** Simple stream representation */
  void toStream(std::ostream& out) const {
    if (d_negated) {
      out << "~";
    }
    out << d_variable;
  }

  /** Hash of the literal */
  size_t hash() const {
    return std::tr1::hash<unsigned>()(index());
  }
};

/** Vector of literals */
typedef std::vector<Literal> LiteralVector;

/** Set of literals */
typedef std::set<Literal> LiteralSet;

class LiteralHashFunction {
public:
  size_t operator () (const Literal& l) const {
    return l.hash();
  }
};

/** Hash-set of literals */
typedef std::hash_set<Literal, LiteralHashFunction> LiteralHashSet;

/** Output operator for a literal */
template<bool refCount>
inline std::ostream& operator << (std::ostream& out, const LiteralRef<refCount>& lit) {
  lit.toStream(out);
  return out;
}

inline std::ostream& operator << (std::ostream& out, const LiteralVector& literals) {
  out << "Literals[";
  for (unsigned i = 0; i < literals.size(); ++ i) {
    if (i > 0) { out << ", "; }
    out << literals[i];
  }
  out << "]";
  return out;
}

inline std::ostream& operator << (std::ostream& out, const LiteralSet& literals) {
  out << "Literals[";
  LiteralSet::const_iterator it = literals.begin();
  bool first = true;
  for (; it != literals.end(); ++ it) {
    if (!first) { out << ", " << *it; }
    else { out << *it; first = false; }
  }
  out << "]";
  return out;
}

inline std::ostream& operator << (std::ostream& out, const LiteralHashSet& literals) {
  out << "Literals[";
  LiteralHashSet::const_iterator it = literals.begin();
  bool first = true;
  for (; it != literals.end(); ++ it) {
    if (!first) { out << ", " << *it; }
    else { out << *it; first = false; }
  }
  out << "]";
  return out;
}

template<bool refCount>
const LiteralRef<refCount> LiteralRef<refCount>::null;

BOOST_STATIC_ASSERT(sizeof(Literal) == sizeof(Literal_Strong));

}
}

namespace std {

template<>
inline void swap(CVC4::mcsat::Literal_Strong& l1, CVC4::mcsat::Literal_Strong& l2) {
  std::swap(l1.d_variable, l2.d_variable);
  std::swap(l1.d_negated, l2.d_negated);
}

}
