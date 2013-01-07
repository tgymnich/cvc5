#include "clause.h"

#include <algorithm>
#include <tr1/functional>

using namespace CVC4;
using namespace CVC4::mcsat;

Clause::Clause(const std::vector<Literal>& literals, size_t ruleId)
: d_size(literals.size())
, d_refCount(0)
, d_ruleId(ruleId)
{
  for (unsigned i = 0; i < literals.size(); ++ i) {
    new (d_literals + i) literal_type(literals[i]);
  }
}

Clause::Clause(const Clause& clause) 
: d_size(clause.size())
, d_refCount(0)
, d_ruleId(clause.d_ruleId) 
{
  for (unsigned i = 0; i < clause.size(); ++ i) {
    new (d_literals + i) literal_type(clause[i]);
  }
}

Clause::~Clause() {
  for (unsigned i = 0; i < size(); ++ i) {
    d_literals[i].~literal_type();
  }
}

void Clause::toStream(std::ostream& out) const {
  out << "[";
  for(unsigned i = 0; i < d_size; ++i) {
    if(i > 0)
      out << " ";
    out << d_literals[i];
  }
  out << "]";
}
