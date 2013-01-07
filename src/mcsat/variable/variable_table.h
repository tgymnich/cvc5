#pragma once

#include "mcsat/variable/variable.h"
#include "mcsat/variable/variable_db.h"

#include <vector>

namespace CVC4 {
namespace mcsat {

template <typename T>
class variable_table {

  typedef std::vector<T> per_type_table;

  std::vector<per_type_table> d_table;

  /** Default value for the vectors */
  T d_defaultValue;

  /** On every new variable the table is resized */
  class new_literal_listener : public VariableDatabase::INewVariableNotify {
    variable_table& d_table;
  public:
    new_literal_listener(variable_table& table)
    : INewVariableNotify(false)
    , d_table(table)
    {}
    void newVariable(Variable var);
  } d_new_literal_listener;

public:

  /** Construct the table. Attaches to the current variable database */
  variable_table(const T& defaultValue = T());

  /** Dereference */
  const T& operator [] (Variable var) const {
    return d_table[var.typeIndex()][var.index()];
  }

  /** Dereference */
  T& operator [] (Variable var) {
    return d_table[var.typeIndex()][var.index()];
  }

  /** Size of the table for a particular type */
  size_t size(size_t typeId) const {
    return d_table[typeId].size();
  }

};

template<typename T>
void variable_table<T>::new_literal_listener::newVariable(Variable var) {
  size_t typeIndex = var.typeIndex();
  if (d_table.d_table.size() <= typeIndex) {
    d_table.d_table.resize(typeIndex + 1);
  }
  size_t index = var.index();
  if (d_table.d_table[typeIndex].size() <= index) {
    d_table.d_table[typeIndex].resize(index + 1, d_table.d_defaultValue);
  }
}

template<typename T>
variable_table<T>::variable_table(const T& defaultValue)
: d_defaultValue(defaultValue)
, d_new_literal_listener(*this)
{
  VariableDatabase* db = VariableDatabase::getCurrentDB();
  db->addNewVariableListener(&d_new_literal_listener);
}

}
}




