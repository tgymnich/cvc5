#pragma once

#include <vector>

#include "mcsat/clause/literal.h"
#include "mcsat/clause/clause_ref.h"

namespace CVC4 {
namespace mcsat {
  
class WatchListManager {

  typedef std::vector<CRef> clause_list;
  typedef std::vector<clause_list> watch_lists;
  
  /** Watchlist indexed by literals */
  watch_lists d_watchLists;

  /** Whether the list needs cleanup */
  std::vector<bool> d_needsCleanup;
  
  /** Resize the watch table to fit the literal */
  void resizeToFit(Literal l) {
    if (l.index() >= d_watchLists.size()) {
      d_watchLists.resize(l.index() + 1);
      d_needsCleanup.resize(l.index() + 1);
    }
  }

public:

  void needsCleanup(Literal lit) {
    resizeToFit(lit);
    d_needsCleanup[lit.index()] = true;
  }

  void add(Literal lit, CRef cRef) {
    resizeToFit(lit);
    d_watchLists[lit.index()].push_back(cRef);
  }
  
  void clean();

  class remove_iterator {
    
    friend class WatchListManager;
    
    /** The watch-lists from the manager */
    watch_lists& d_watchLists;     
    
    /** Index of the watch list */
    size_t d_list_index;

    /** Element beyon the last to keep */
    size_t d_end_of_keep;
    /** Current element */
    size_t d_current;
 
    remove_iterator(watch_lists& lists, size_t index)
    : d_watchLists(lists)
    , d_list_index(index)
    , d_end_of_keep(0)
    , d_current(0)
    {}
 
  public:
    
    /** Is there a next element */
    bool done() const {
      return d_current == d_watchLists[d_list_index].size();
    }
    /** Continue to the next element and keep this one */
    void next_and_keep() {
      d_watchLists[d_list_index][d_end_of_keep ++] = d_watchLists[d_list_index][d_current ++];
    }
    /** Continue to the next element and remove this one */
    void next_and_remove() { 
      ++ d_current;
    }
    /** Get the current element */
    CRef operator * () const {
      return d_watchLists[d_list_index][d_current];
    }
    /** Removes removed elements from the list */
    ~remove_iterator() {
      while (!done()) {
	next_and_keep();
      }
      d_watchLists[d_list_index].resize(d_end_of_keep);
    }
  };

  /** Returns the iterator that can remove elements */
  remove_iterator begin(Literal l) {
    resizeToFit(l);
    return remove_iterator(d_watchLists, l.index());
  }
};

}
}
