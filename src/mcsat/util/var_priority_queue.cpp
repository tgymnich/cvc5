#include "mcsat/util/var_priority_queue.h"
#include "mcsat/variable/variable_db.h"

using namespace CVC4;
using namespace mcsat;
using namespace util;

VariablePriorityQueue::VariablePriorityQueue(size_t typeIndex)
: d_typeIndex(typeIndex)
, d_variableScoresMax(1)
, d_variableScoreCmp(d_variableScores)
, d_variableQueue(d_variableScoreCmp)
, d_variableScoreIncreasePerBump(1)
, d_maxScoreBeforeScaling(1e100)
{
}

void VariablePriorityQueue::newVariable(Variable var, double score) {
  Assert(var.typeIndex() == d_typeIndex);

  // Insert a new score (max of the current scores)
  d_variableScores.resize(var.index() + 1, score);
  d_variableScores[var.index()] = score;
  // Make sure that there is enough space for the pointer
  d_variableQueuePositions.resize(var.index() + 1);

  if (score > d_variableScoresMax) {
    d_variableScoresMax = score;
  }

  // Enqueue the variable
  enqueue(var);
}

bool VariablePriorityQueue::inQueue(Variable var) const {
  Assert(d_typeIndex == var.typeIndex());
  Assert(var.index() < d_variableScores.size());
  return d_variableQueuePositions[var.index()] != variable_queue::point_iterator();
}

void VariablePriorityQueue::enqueue(Variable var) {
  Assert(d_typeIndex == var.typeIndex());
  Assert(var.index() < d_variableScores.size());
  // Add to the queue
  variable_queue::point_iterator it = d_variableQueue.push(var);
  d_variableQueuePositions[var.index()] = it;
}


void VariablePriorityQueue::bumpVariable(Variable var) {
  Assert(d_typeIndex == var.typeIndex());
  Assert(var.index() < d_variableScores.size());
  // New heuristic value
  double newValue = d_variableScores[var.index()] + d_variableScoreIncreasePerBump;
  if (newValue > d_variableScoresMax) {
    d_variableScoresMax = newValue;
  }

  if (inQueue(var)) {
    // If the variable is in the queue, erase it first
    d_variableQueue.erase(d_variableQueuePositions[var.index()]);
    d_variableScores[var.index()] = newValue;
    enqueue(var);
  } else {
    // Otherwise just update the value
    d_variableScores[var.index()] = newValue;
  }

  // If the new value is too big, update all the values
  if (newValue > d_maxScoreBeforeScaling) {
    // This preserves the order, we're fine
    for (unsigned i = 0, i_end = d_variableScores.size(); i < i_end; ++ i) {
      d_variableScores[i] *= d_maxScoreBeforeScaling;
    }
    // TODO: decay activities
    // variableHeuristicIncrease *= 1e-100;
    d_variableScoresMax *= d_maxScoreBeforeScaling;
  }
}

double VariablePriorityQueue::getScore(Variable var) const {
  Assert(d_typeIndex == var.typeIndex());
  Assert(var.index() < d_variableScores.size());
  return d_variableScores[var.index()];
}

Variable VariablePriorityQueue::pop() {
  Variable var = d_variableQueue.top();
  d_variableQueue.pop();
  d_variableQueuePositions[var.index()] = variable_queue::point_iterator();
  return var;
}

bool VariablePriorityQueue::empty() const {
  return d_variableQueue.empty();
}

void VariablePriorityQueue::clear() {
  d_variableScores.clear();
  d_variableScoresMax = 0;
  d_variableQueue.clear();
  d_variableQueuePositions.clear();
}

void VariablePriorityQueue::gcRelocate(const VariableGCInfo& vReloc) {

  VariableGCInfo::const_iterator it = vReloc.begin(d_typeIndex);
  VariableGCInfo::const_iterator it_end = vReloc.begin(d_typeIndex);

  for (; it != it_end; ++ it) {
    Variable erased = *it;
    if (inQueue(erased)) {
      d_variableQueue.erase(d_variableQueuePositions[erased.index()]);
      d_variableQueuePositions[erased.index()] = variable_queue::point_iterator();
    }
    d_variableScores[erased.index()] = 0;
  }

}
