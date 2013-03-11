#pragma once

#include "cvc4_private.h"

#include <cstddef>
#include <iostream>
#include <boost/integer/integer_mask.hpp>

#include "mcsat/clause/clause.h"

namespace CVC4 {
namespace mcsat {

class ClauseDatabase;

/** A reference to a clause */
template<bool refCount>
class ClauseRef {

  /** Number of bits kept for the reference */
  static const size_t BITS_FOR_REFERENCE = 32;
  
  /** NUmber of bits kept for the database id */
  static const size_t BITS_FOR_DATABASE  = 32;

  /** Null reference */
  static const size_t nullRef = boost::low_bits_mask_t<BITS_FOR_REFERENCE>::sig_bits;

private:
 
  friend class ClauseRef<!refCount>;

  /** Reference into the clause database */
  size_t d_ref : BITS_FOR_REFERENCE;
  
  /** Id of the clause database */
  size_t d_db  : BITS_FOR_DATABASE;
  
  /** 
   * Construct the reference. This one is not reference counted, as only the
   * clause database creates these.
   */
  ClauseRef(size_t ref, size_t db) 
  : d_ref(ref) 
  , d_db(db)
  {}

  /** Only clause database can create the references */
  friend class ClauseDatabase;

  /** Does this ref correspond to an actual clause */
  bool hasClause() const {
    if (d_ref == nullRef) return false;
    return true;
  }

public:

  /**
   * Copy constructor of the same type.
   */
  ClauseRef(const ClauseRef& other)
  : d_ref(other.d_ref)
  , d_db(other.d_db) {
    if (refCount && hasClause()) {
      getClause().incRefCount();
    }
  }

  /**
   * Copy constructor of the other type.
   */
  ClauseRef(const ClauseRef<!refCount>& other)
  : d_ref(other.d_ref) 
  , d_db(other.d_db) {
    if (refCount && hasClause()) {
      getClause().incRefCount();
    }
  }

  /** Assignment operator */
  ClauseRef& operator = (const ClauseRef& other) {
    if (this != &other) {
      if (refCount && hasClause()) {
	getClause().decRefCount();
      }
      d_ref = other.d_ref;
      d_db = other.d_db;
      if (refCount && hasClause()) {
	getClause().incRefCount();
      }
    }
    return *this;
  }
  
  /** Assignment operator */
  ClauseRef& operator = (const ClauseRef<!refCount>& other) {
    if (refCount && hasClause()) {
      getClause().decRefCount();
    }
    d_ref = other.d_ref;
    d_db = other.d_db;
    if (refCount && hasClause()) {
      getClause().incRefCount();
    }
    return *this;
  }

  /** Decrease the reference counts if needed */
  ~ClauseRef() {
    if (refCount && hasClause()) {
      getClause().decRefCount();
    }
  }

  /** Default constructs null */
  ClauseRef() : d_ref(nullRef), d_db(0) {}

  /** The null clause reference */
  static const ClauseRef null;

  /** Return the id of the database holding this clause */
  size_t getDatabaseId() const {
    return d_db;
  }
  
  /** Get the actual clause */
  Clause& getClause() const;

  /** Check if the reference is null */
  bool isNull() const;
  
  /** Is this clause reference in use */
  bool inUse() const {
    return getClause().inUse();
  }
  
  /** Print the clause to the stream */
  void toStream(std::ostream& out) const;

  /** Hash the reference */
  size_t hash() const {
    return std::tr1::hash<size_t>()(d_ref);
  }

  /** Compare two references */
  template<bool rc2>
  bool operator == (const ClauseRef<rc2>& other) const;

  /** Compare two references (by pointer value) */
  bool operator < (const ClauseRef& other) const {
    if (d_db == other.d_db) {
      return d_ref < other.d_ref;
    } else {
      return d_db < other.d_db;
    }
  }
};

/** Non reference counted reference */
typedef ClauseRef<false> CRef;
/** Non reference counted reference */
typedef ClauseRef<true> CRef_Strong;

template<bool refCount>
const ClauseRef<refCount> ClauseRef<refCount>::null;

template<bool refCount>
bool ClauseRef<refCount>::isNull() const {
  return *this == null;
}

template<bool rc1>
template<bool rc2>
bool ClauseRef<rc1>::operator == (const ClauseRef<rc2>& other) const {
  return d_ref == other.d_ref;
}

template<bool refCount>
inline std::ostream& operator << (std::ostream& out, const ClauseRef<refCount>& cRef) {
  return out << cRef.getClause();
}

struct CRefHashFunction {
  size_t operator () (const CRef& cRef) const {
    return cRef.hash();
  }
};


} /* namespace mcsat */
} /* namespace CVC4 */


