#include "variable.h"
#include "variable_db.h"

using namespace CVC4;
using namespace CVC4::mcsat;

template<bool refCount>
const VariableRef<refCount> VariableRef<refCount>::null;

template<bool refCount>
TNode VariableRef<refCount>::getNode() const {
  return VariableDatabase::getCurrentDB()->getNode(*this);
}

template<bool refCount>
TypeNode VariableRef<refCount>::getTypeNode() const {
  return VariableDatabase::getCurrentDB()->getTypeNode(*this);
}

template<bool refCount>
void VariableRef<refCount>::incRefCount() const {
  VariableDatabase::getCurrentDB()->attach(*this);
}

template<bool refCount>
void VariableRef<refCount>::decRefCount() const {
  VariableDatabase::getCurrentDB()->detach(*this);
}

template<bool refCount>
bool VariableRef<refCount>::inUse() const {
  if (isNull()) return false;
  return VariableDatabase::getCurrentDB()->inUse(*this);  
}

template<bool refCount>
void VariableRef<refCount>::toStream(std::ostream& out) const {

  if (isNull()) {
    out << "null";
    return;
  }

  // Get the node
  TNode node = getNode();

  // If the node is not null that's what we output
  if (!node.isNull()) {
    out << node;
    return;
  }

  /** Internal variable */
  out << "m" << d_varId;
}

// Explicit instatiations
template class VariableRef<true>;
template class VariableRef<false>;
