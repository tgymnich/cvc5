#pragma once

#include "expr/node.h"
#include "context/cdo.h"
#include "mcsat/variable/variable.h"
#include "mcsat/fm/linear_constraint.h"

#include <iostream>
#include <boost/integer_traits.hpp>

namespace CVC4 {
namespace mcsat {
namespace fm {

/** Map from variables to constraints */
typedef std::hash_map<Variable, LinearConstraint, VariableHashFunction> var_to_constraint_map;

/** Value info for stating x != value */
struct DisequalInfo {
  /** The value itself */
  Rational value;
  /** The reason for this bound */
  Variable reason;

  DisequalInfo() {}

  DisequalInfo(Rational value, Variable reason)
  : value(value), reason(reason)
  {}

  void toStream(std::ostream& out) const {
    out << value;
  }
};

inline std::ostream& operator << (std::ostream& out, const DisequalInfo& info) {
  info.toStream(out);
  return out;
}

/** Bound information */
struct BoundInfo {
  /** The value of the bound */
  Rational value;
  /** Is the bound strict (<,>) */
  bool strict;
  /** What is the reason for this bound */
  Variable reason;

  BoundInfo()
  : strict(false) {}

  BoundInfo(Rational value, bool strict, Variable reason)
  : value(value), strict(strict), reason(reason) {}

  bool improvesLowerBound(const BoundInfo& other) const {
    // x > value is better than x > other.value
    // if value > other.value or they are equal but this one
    // is strict    
    int cmp = value.cmp(other.value);
    return (cmp > 0 || (cmp == 0 && strict));
  }
  
  bool improvesUpperBound(const BoundInfo& other) const {
    // x < value is better than x < other.value
    // if value < other.value or they are equal but this one 
    // is strict
    int cmp = value.cmp(other.value);
    return (cmp < 0 || (cmp == 0 && strict));    
  }
  
  static bool inConflict(const BoundInfo& lower, const BoundInfo& upper) {
    // x > a and x < b are in conflict 
    // if a > b or they are equal but one is strict
    int cmp = lower.value.cmp(upper.value);
    return (cmp > 0 || (cmp == 0 && (lower.strict || upper.strict)));
  }

  void toStream(std::ostream& out) const {
    out << "[" << value << (strict ? "" : "=") << "]";
  }
};

inline std::ostream& operator << (std::ostream& out, const BoundInfo& info) {
  info.toStream(out);
  return out;
}

/** Context-dependent bounds model */
class CDBoundsModel : public context::ContextNotifyObj {

  /** Index of the bound in the bounds vector */
  typedef unsigned BoundInfoIndex;
  
  /** Null bound index */
  static const BoundInfoIndex null_bound_index = boost::integer_traits<BoundInfoIndex>::const_max;
  
  /** Map from variables to the index of the bound information in the bound trail */
  typedef std::hash_map<Variable, BoundInfoIndex, VariableHashFunction> bound_map;
    
  /** Lower bounds map */
  bound_map d_lowerBounds;
  
  /** Upper bounds map */
  bound_map d_upperBounds;
  
  /** Bound update trail (this is where the actual bounds are) */
  std::vector<BoundInfo> d_boundTrail;

  /** Information for undoing */
  struct UndoBoundInfo {
    /** Which variable to undo */
    Variable var;
    /** Index of the previous bound (null if none) */
    BoundInfoIndex prev;
    /** Is it a lower bound */
    bool isLower;
    
    UndoBoundInfo()
    : prev(null_bound_index), isLower(false) {}
    
    UndoBoundInfo(Variable var, BoundInfoIndex prev, bool isLower)
    : var(var), prev(prev), isLower(isLower) {}      
  };
  
  /** Bound update trail (same size as d_boundTrail) */
  std::vector<UndoBoundInfo> d_boundTrailUndo;
  
  /** Count of lower bound updates */
  context::CDO<unsigned> d_boundTrailSize;

  /** Index of the value in the value list */
  typedef unsigned DisequalInfoIndex;

  /** Null value list index index */
  static const DisequalInfoIndex null_diseqal_index = boost::integer_traits<DisequalInfoIndex>::const_max;

  /** Map from variables to the head of their value list */
  typedef std::hash_map<Variable, DisequalInfoIndex, VariableHashFunction> disequal_map;

  /** The map from variables to it's diseqality lists */
  disequal_map d_disequalValues;

  /** Trail of disequality information */
  std::vector<DisequalInfo> d_disequalTrail;

  /** Var != value, and link to the previous dis-equality */
  struct UndoDisequalInfo {
    /** Which variable */
    Variable var;
    /** INdex of the previsous disequal info */
    DisequalInfoIndex prev;

    UndoDisequalInfo()
    : prev(null_diseqal_index) {}

    UndoDisequalInfo(Variable var, DisequalInfoIndex prev)
    : var(var), prev(prev) {}

  };

  /** Values */
  std::vector<UndoDisequalInfo> d_disequalTrailUndo;

  /** Size of the values trail */
  context::CDO<unsigned> d_disequalTrailSize;

  /** Variables that are in conflict */
  std::set<Variable> d_variablesInConflict;
  
  /** Update to the appropriate context state */
  void contextNotifyPop();
  
  /**
   * Is the value in the range of the bounds.
   * @oaram onlyOption if in range and this is the only available option => true
   */
  bool inRange(Variable var, Rational value, bool& onlyOption) const;

  /**
   * Check whether variable is asserted to be different from the given constant.
   */
  bool isDisequal(Variable var, Rational value) const;

  /**
   * Get the set of all the values that are asserted to be disequal from var.
   */
  void getDisequal(Variable var, std::set<Rational>& disequal) const;

public:
  
  ~CDBoundsModel() throw(AssertionException) {}
  
  /** Construct it */
  CDBoundsModel(context::Context* context);
  
  /** Update the lower bound, returns true if variable is now fixed */
  bool updateLowerBound(Variable var, const BoundInfo& info);

  /** Update the upper bound, returns true if variable is now fixed */
  bool updateUpperBound(Variable var, const BoundInfo& info);

  /** Adds the value to the list of values that a variable must be disequal from */
  void addDisequality(Variable var, const DisequalInfo& info);

  /** Does the variable have a lower bound */
  bool hasLowerBound(Variable var) const;

  /** Does the variable have an upper bound */
  bool hasUpperBound(Variable var) const;
  
  /** Pick a value for a variable */
  Rational pick(Variable var) const;

  /** Get the current lower bound info */
  const BoundInfo& getLowerBoundInfo(Variable var) const;

  /** Get the current upper bound info */
  const BoundInfo& getUpperBoundInfo(Variable var) const;
  
  /** Get the information about the disequality */
  const DisequalInfo& getDisequalInfo(Variable var, Rational value) const;

  /** Is the state of bounds in conflict */
  bool inConflict() const {
    return d_variablesInConflict.size() > 0;
  }
  
  /** Get the variables with conflicting bound */
  void getVariablesInConflict(std::set<Variable>& out) {
    out.swap(d_variablesInConflict);
  }
};

} // fm namespace
}
}



