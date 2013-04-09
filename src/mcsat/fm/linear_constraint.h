#pragma once

#include "expr/node.h"
#include "mcsat/variable/variable.h"
#include "mcsat/clause/literal.h"

#include <vector>
#include <iostream>

namespace CVC4 {
namespace mcsat {

class SolverTrail;

namespace fm {

/** Map from variables to coefficients, null is the constant term */
typedef std::hash_map<Variable, Rational, VariableHashFunction> var_to_rational_map;

/**
 * A generic linear constraint (t op 0) is just a map from variables to coefficients
 * representing a term t, and a kind of constraint op. The resulting constraint will
 * be of kind either >=, >, ==, or !=.
 */
class LinearConstraint {

  /** Map from variables to coefficients */
  var_to_rational_map d_coefficients;
  /** The kind of constraint >=, ... */
  Kind d_kind;

  static bool parse(TNode constraint, Rational mult, var_to_rational_map& coefficientMap);

public:

  /** Default constructor */
  LinearConstraint(): d_kind(kind::LAST_KIND) {
    d_coefficients[Variable::null] = 0;
  }

  /** Clears the constraint */
  void clear() {
    d_coefficients.clear();
    d_coefficients[Variable::null] = 0;
    d_kind = kind::LAST_KIND;
  }

  /** Returns the number of proper variables */
  unsigned size() {
    return d_coefficients.size() - 1;
  }

  /** Get the variables of this constraint */
  void getVariables(std::vector<Variable>& vars);

  /** Get the kind of the constraint */
  Kind getKind() const {
    return d_kind;
  }

  /** Swap with the given constraint */
  void swap(LinearConstraint& c);

  /** Output to stream */
  void toStream(std::ostream& out) const;

  /**
   * Evaluate the constraint in the trail (true, false). It returns the level at which
   * the constraint evaluates.
   */
  bool evaluate(const SolverTrail& trail, unsigned& level) const;

  typedef var_to_rational_map::const_iterator const_iterator;

  /** Returns the cosfficient with the given variable */
  Rational getCoefficient(Variable var) const;
  
  const_iterator begin() const {
    return d_coefficients.begin();
  }

  const_iterator end() const {
    return d_coefficients.end();
  }

  /**
   * Return the literal representation of the constraint. If the literal does 
   * not exist yet, it will be created hence spawning notifications.
   */
  Literal getLiteral() const;
  
  /** 
   * Multiply the constraint with the given positive constant.
   */
  void multiply(Rational c);
  
  /** 
   * Add the given linear constraint multiplied with the given positive 
   * factor.
   */
  void add(const LinearConstraint& other, Rational c = 1);
  
  /** 
   * Parse a literal into a linear constraint. Returns true if successful. The
   * result of the parse will always be a constraint of type t > 0, t >= 0, 
   * t = 0, ot t != 0, i.e. t < 0 and t <= are parsed in as -t > 0 and -t >= 0. 
   */
  static bool parse(Literal constraint, LinearConstraint& out);

  /** Negates the kind as if the linear constraint was negated */
  static Kind negateKind(Kind kind);

  /** Flips the kind as if the linear constraint was multiplied with -1 */
  static Kind flipKind(Kind kind);

};

inline std::ostream& operator << (std::ostream& out, const LinearConstraint& c) {
  c.toStream(out);
  return out;
}


}
}
}





