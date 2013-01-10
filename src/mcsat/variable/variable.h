#pragma once

#include "cvc4_private.h"

#include <vector>
#include <boost/integer/integer_mask.hpp>

#include "expr/node.h"

namespace CVC4 {
namespace mcsat {

template <bool refCount>
class VariableRef;
  
/** Variable with reference counting */
typedef VariableRef<true> Variable_Strong;
/** Weak variable */
typedef VariableRef<false> Variable;

/** 
 * A variable from a variable database. If refCount is true the references are
 * counted in the variable database. All variables with referenc count 0 will can 
 * be reclaimed at search level 0. 
 */
template <bool refCount>
class VariableRef {

public:
  
  /** Number of bits for the variable id */
  static const size_t BITS_FOR_VARIABLE_ID = 32;
  static const size_t BITS_FOR_TYPE_ID = 32;
  
  /** The null variable id */
  static const size_t s_null_varId = boost::low_bits_mask_t<BITS_FOR_VARIABLE_ID>::sig_bits;
  static const size_t s_null_typeId = boost::low_bits_mask_t<BITS_FOR_TYPE_ID>::sig_bits;

private:
  
  friend class VariableDatabase;
  friend class VariableRef<!refCount>;

  friend void std::swap<>(VariableRef& v1, VariableRef& v2);

  /** Id of the variable */
  size_t d_varId     : BITS_FOR_VARIABLE_ID;
  size_t d_typeId    : BITS_FOR_TYPE_ID;

  /** Make a variable */
  VariableRef(size_t varId, size_t typeId)
  : d_varId(varId), d_typeId(typeId)
  {}

  /** Increase the reference count */
  void incRefCount() const;

  /** Decrease the reference count */
  void decRefCount() const;
  
public:

  /** The null variable */
  static const VariableRef null;

  /** Is this a null variable */
  bool isNull() const {
    return d_varId == s_null_varId;
  }

  /** Creates a null variable */
  VariableRef()
  : d_varId(s_null_varId)
  , d_typeId(s_null_typeId)
  {}

  ~VariableRef() {
    if (refCount && !isNull()) {
      decRefCount();
    }
  }
  
  /** Copy constructor */
  VariableRef(const VariableRef& other) 
  : d_varId(other.d_varId)
  , d_typeId(other.d_typeId)
  {
    if (refCount && !isNull()) {
	incRefCount();
    }
  }

  /** Copy constructor */
  VariableRef(const VariableRef<!refCount>& other) 
  : d_varId(other.d_varId)
  , d_typeId(other.d_typeId)
  {
    if (refCount && !isNull()) {
      incRefCount();
    }
  }

  /** Assignment operator */
  VariableRef& operator = (const VariableRef& other) {
    if (this != &other) {
      if (refCount && !isNull()) {
	decRefCount();
      }
      d_varId = other.d_varId;
      d_typeId = other.d_typeId;
      if (refCount && !isNull()) {
	incRefCount();
      }
    }
    return *this;
  }
  
  /** Get the node associated with this variable (if any) */
  TNode getNode() const;

  /** Returns the type of the variable (if any) */
  TypeNode getTypeNode() const;

  /** Returns true if the variable is in use  */
  bool inUse() const;
  
  /** Output to stream */
  void toStream(std::ostream& out) const;

  /** Comparison operator */
  bool operator < (const VariableRef& other) const {
    return d_varId < other.d_varId;
  }

  /** Comparison operator */
  bool operator == (const VariableRef& other) const {
    return d_varId == other.d_varId;
  }

  /** Comparison operator */
  bool operator != (const VariableRef& other) const {
    return d_varId != other.d_varId;
  }
  
  /** Return the 0-based index of the variable of it's type */
  size_t index() const {
    return d_varId;
  }
  
  /** Return the 0-based index of the variables type */
  size_t typeIndex() const {
    return d_typeId;
  }

  /** Swap with the given variable */
  void swap(VariableRef& var) {
    size_t tmp;
    tmp = d_varId; d_varId = var.d_varId; var.d_varId = tmp;
    tmp = d_typeId; d_typeId = var.d_typeId; var.d_typeId = tmp;
  }

};

/** Output operator for variables */
template<bool refCount>
inline std::ostream& operator << (std::ostream& out, const VariableRef<refCount>& var) {
  var.toStream(out); 
  return out;
}

} /* Namespace mcsat */
} /* Namespace CVC4 */

namespace std {

template<>
inline void swap(CVC4::mcsat::Variable_Strong& v1, CVC4::mcsat::Variable_Strong& v2) {
  v1.swap(v2);
}

}
