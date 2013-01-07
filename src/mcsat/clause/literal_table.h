#pragma once

#include "mcsat/clause/literal.h"
#include "mcsat/variable/variable_db.h"

#include <vector>

namespace CVC4 {
namespace mcsat {

template <typename T>
class literal_table {

  std::vector<T> d_table;

  /** Default value for the vectors */
  T d_defaultValue;

  /** On every new variable the table is resized */
  class new_literal_listener : public VariableDatabase::INewVariableNotify {
    literal_table& d_table;
    size_t d_bool_type_index;
  public:
    new_literal_listener(literal_table& table)
    : INewVariableNotify(false)
    , d_table(table)
    , d_bool_type_index(VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->booleanType()))
    {}
    void newVariable(Variable var);
  } d_new_literal_listener;

public:

  /** Construct the table. Attaches to the current variable database */
  literal_table(const T& defaultValue = T());

  /** Dereference */
  const T& operator [] (Literal l) const {
    return d_table[l.index()];
  }

  /** Dereference */
  T& operator [] (Literal l) {
    return d_table[l.index()];
  }

  /** Size of the table */
  size_t size() const {
    return d_table.size();
  }

};

template<typename T>
void literal_table<T>::new_literal_listener::newVariable(Variable var) {
  if (var.typeIndex() == d_bool_type_index) {
    Literal l1(var, true);
    Literal l2(var, false);

    size_t index = std::max(l1.index(), l2.index());
    if (d_table.d_table.size() <= index) {
      d_table.d_table.resize(index + 1, d_table.d_defaultValue);
    }
  }
}

template<typename T>
literal_table<T>::literal_table(const T& defaultValue)
: d_defaultValue(defaultValue)
, d_new_literal_listener(*this)
{
  VariableDatabase* db = VariableDatabase::getCurrentDB();
  db->addNewVariableListener(&d_new_literal_listener);
}

}
}




