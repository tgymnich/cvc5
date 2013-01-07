#include "mcsat/watch_list_manager.h"

using namespace CVC4;
using namespace mcsat;

void WatchListManager::clean() {
  for(unsigned list = 0, list_end = d_watchLists.size(); list < list_end; ++ list) {
    if (d_needsCleanup[list]) {
      remove_iterator it(d_watchLists, list);
      while (!it.done()) {
	if ((*it).inUse()) {
	  it.next_and_keep();
	} else {
	  it.next_and_remove();
	}
      }
      d_needsCleanup[list] = false;
    }
  }
}