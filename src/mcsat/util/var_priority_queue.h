#pragma once

#include "mcsat/variable/variable.h"
#include "mcsat/variable/variable_db.h"

#include <ext/pb_ds/priority_queue.hpp>

namespace CVC4 {
namespace mcsat {
namespace util {

/**
 * A priority queue for the variables of certain types.
 */
class VariablePriorityQueue {

  /** Scores of variables */
  std::vector< std::vector<double> > d_variableScores;

  /** Max over the score of all the variables */
  double d_variableScoresMax;

  /** Compare variables according to their current score */
  class VariableScoreCmp {
    std::vector< std::vector<double> >& d_variableScores;
  public:
    VariableScoreCmp(std::vector< std::vector<double> >& variableScores)
    : d_variableScores(variableScores) {}
    bool operator() (const Variable& v1, const Variable& v2) const {
      double cmp = d_variableScores[v1.typeIndex()][v1.index()] - d_variableScores[v1.typeIndex()][v2.index()];
      if (cmp == 0) {
        return v1 < v2;
      } else {
        return cmp < 0;
      }

    }
  } d_variableScoreCmp;

  /** Priority queue type we'll be using to keep the variables to pick */
  typedef __gnu_pbds::priority_queue<Variable, VariableScoreCmp> variable_queue;

  /** Priority queue for the variables */
  variable_queue d_variableQueue;

  /** Position in the variable queue */
  std::vector< std::vector<variable_queue::point_iterator> > d_variableQueuePositions;

  /** How much to increase the score per bump */
  double d_variableScoreIncreasePerBump;

  /** Max score before we scale everyone back */
  double d_maxScoreBeforeScaling;

public:

  /** Construct a variable queue for variables of given type */
  VariablePriorityQueue();

  /** Add new variable to track */
  void newVariable(Variable var) {
    newVariable(var, d_variableScoresMax);
  }

  /** Add new variable to track with the given score */
  void newVariable(Variable var, double score);

  /** Get the top variable */
  Variable pop();

  /** Is the queue empty */
  bool empty() const;

  /** Enqueues the variable for decision making */
  void enqueue(Variable var);

  /** Is the variable in queue */
  bool inQueue(Variable var) const;

  /** Bump the score of the variable */
  void bumpVariable(Variable var);

  /** Get the score of a variable */
  double getScore(Variable var) const;

  /** Relocates any the variable queue */
  void gcRelocate(const VariableGCInfo& vReloc);
};

}
}
}




