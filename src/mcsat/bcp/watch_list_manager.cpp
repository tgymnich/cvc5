#include "mcsat/bcp/watch_list_manager.h"

using namespace CVC4;
using namespace mcsat;

void WatchListManager::gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc) {

  size_t typeIndex = VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->booleanType());

  if (vReloc.size(typeIndex) > 0) {
    // Clear all the lists for erased variables
    VariableGCInfo::const_iterator it = vReloc.begin(typeIndex);
    VariableGCInfo::const_iterator it_end = vReloc.end(typeIndex);
    for (; it != it_end; ++ it) {
      Variable var = *it;
      Literal varP(var, false);
      Literal varN(var, true);

      // Clear the watchlist
      d_watchLists[varP].clear();
      d_watchLists[varN].clear();

      // Don't need cleanup
      d_needsCleanup[varP] = false;
      d_needsCleanup[varP] = false;
    }
  }
  // Clean the rest of the lists
  for (unsigned i = 0; i < d_watchLists.size(); ++ i) {
    if (d_watchLists[i].size() > 0) {
      cReloc.relocate(d_watchLists[i]);
    }
  }

}
