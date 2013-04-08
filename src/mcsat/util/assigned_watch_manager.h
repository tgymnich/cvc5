#pragma once

#include <vector>
#include <boost/integer_traits.hpp>
#include <ext/hash_map>

namespace CVC4 {
namespace mcsat {
namespace util {

class VariableListReference {

protected:

  /** Number of bits kept for the reference */
  static const size_t BITS_FOR_REFERENCE = 32;

  /** NUmber of bits kept for the size */
  static const size_t BITS_FOR_SIZE = 32;

  /** Index of the reference */
  size_t d_index : BITS_FOR_REFERENCE;

  /** Size of the list */
  size_t d_size  : BITS_FOR_SIZE;

  /** Null reference */
  static const size_t nullRef = boost::low_bits_mask_t<BITS_FOR_REFERENCE>::sig_bits;

  VariableListReference(size_t ref, size_t d_size)
  : d_index(ref)
  , d_size(0)
  {}

  friend class AssignedWatchManager;

public:

  VariableListReference()
  : d_index(nullRef)
  , d_size(0)
  {}

  VariableListReference(const VariableListReference& other)
  : d_index(other.d_index)
  , d_size(other.d_size)
  {}

  VariableListReference& operator = (const VariableListReference& other) {
    d_index = other.d_index;
    return *this;
  }

  size_t index() const {
    return d_index;
  }

  size_t size() const {
    return d_size;
  }

  bool operator == (const VariableListReference& other) const {
    return d_index == other.d_index;
  }

};

class VariableList : public VariableListReference {

  std::vector<Variable>& d_memory;

public:

  VariableList(std::vector<Variable>& memory, VariableListReference ref)
  : VariableListReference(ref)
  , d_memory(memory)
  {}

  VariableList(const VariableList& other)
  : VariableListReference(other)
  , d_memory(other.d_memory)
  {}

  const Variable& operator [] (size_t i) const {
    return d_memory[d_index + i];
  }

  void swap(size_t i, size_t j) {
    std::swap(d_memory[d_index + i], d_memory[d_index + j]);
  }

  void toStream(std::ostream& out) const {
    out << "VariableList[";
    for (unsigned i = 0; i < d_size; ++ i) {
      if (i > 0) out << ",";
      out << d_memory[d_index + i];
    }
    out << "]";
  }

};

inline std::ostream& operator << (std::ostream& out, const VariableList& list) {
  list.toStream(out);
  return out;
}

class AssignedWatchManager {

  /** The list memory */
  std::vector<Variable> d_memory;

  /** A list of references to variable lists */
  typedef std::vector<VariableListReference> watch_list;

  /** A table from variables to lists */
  typedef variable_table<watch_list> watch_lists;

  /** Watchlist indexed by variables */
  watch_lists d_watchLists;

public:

  /** Adds a new list of variables to watch */
  VariableListReference newListToWatch(const std::vector<Variable>& vars) {
    VariableListReference ref(d_memory.size(), vars.size());
    for(unsigned i = 0; i < vars.size(); ++ i) {
      d_memory.push_back(vars[i]);
    }
    // The new clause
    return ref;
  }

  /** Add list to the watchlist of var */
  void watch(Variable var, VariableListReference ref) {
    d_watchLists[var].push_back(ref);
  }

  /** An iterator that can remove */
  class remove_iterator {
    
    friend class AssignedWatchManager;
    
    /** The watch-lists from the manager */
    watch_lists& d_watchLists;     
    
    /** Index of the watch list */
    Variable d_variable;

    /** Element beyond the last to keep */
    size_t d_end_of_keep;
    /** Current element */
    size_t d_current;
 
    remove_iterator(watch_lists& lists, Variable var)
    : d_watchLists(lists)
    , d_variable(var)
    , d_end_of_keep(0)
    , d_current(0)
    {}

  public:
    
    /** Is there a next element */
    bool done() const {
      return d_current == d_watchLists[d_variable].size();
    }
    /** Continue to the next element and keep this one */
    void next_and_keep() {
      d_watchLists[d_variable][d_end_of_keep ++] = d_watchLists[d_variable][d_current ++];
    }
    /** Continue to the next element and remove this one */
    void next_and_remove() { 
      ++ d_current;
    }
    /** Get the current element */
    VariableListReference operator * () const {
      return d_watchLists[d_variable][d_current];
    }

    /** Removes removed elements from the list */
    ~remove_iterator() {
      while (!done()) {
	next_and_keep();
      }
      d_watchLists[d_variable].resize(d_end_of_keep);
    }
  };

  /** Returns the iterator that can remove elements */
  remove_iterator begin(Variable var) {
    return remove_iterator(d_watchLists, var);
  }

  /** Get the actual variable list */
  VariableList get(VariableListReference ref) {
    return VariableList(d_memory, ref);
  }
};

/**
 * This hash function is only apropriate for lists coming from the same memory.
 */
class VariableListReferenceHashFunction {
public:
  size_t operator () (const VariableListReference& varList) const {
    return varList.index();
  }
};

typedef __gnu_cxx::hash_map<VariableListReference, Variable, VariableListReferenceHashFunction> varlist_to_var_map;

}
}
}
