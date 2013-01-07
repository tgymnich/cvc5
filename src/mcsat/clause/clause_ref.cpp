#include "clause_ref.h"
#include "clause_db.h"

using namespace CVC4;
using namespace mcsat;

template<bool refCount>
Clause& ClauseRef<refCount>::getClause() const {
  Assert(hasClause());
  return ClauseFarm::getCurrent()->getClauseDB(d_db).getClause(*this);
}

template class ClauseRef<true>;
template class ClauseRef<false>;
