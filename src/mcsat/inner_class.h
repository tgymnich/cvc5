#pragma once

namespace CVC4 {
namespace mcsat {

/**
 * Simple class to simplify creating Java style inner classes.
 */
template<typename T>
class InnerClass {

protected:

  T& d_outerClass;

  T& outerClass() {
    return d_outerClass;
  }

  InnerClass(T& outerClass)
  : d_outerClass(outerClass)
  {}

};

}
}



