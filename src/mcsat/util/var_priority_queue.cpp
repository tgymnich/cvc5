#include "mcsat/util/var_priority_queue.h"
#include "mcsat/variable/variable_db.h"

using namespace CVC4;
using namespace mcsat;
using namespace util;

VariablePriorityQueue::VariablePriorityQueue()
: d_variableScoresMax(1)
, d_variableScoreCmp(d_variableScores)
, d_variableQueue(d_variableScoreCmp)
, d_variableScoreIncreasePerBump(1)
, d_maxScoreBeforeScaling(1e100)
{
}

void VariablePriorityQueue::newVariable(Variable var, double score) {

  size_t typeIndex = var.typeIndex();
  if (typeIndex >= d_variableScores.size()) {
    d_variableScores.resize(typeIndex + 1);
    d_variableQueuePositions.resize(typeIndex + 1);
  }

  // Insert a new score (max of the current scores)
  d_variableScores[typeIndex].resize(var.index() + 1, score);
  d_variableScores[typeIndex][var.index()] = score;
  // Make sure that there is enough space for the pointer
  d_variableQueuePositions[typeIndex].resize(var.index() + 1);

  if (score > d_variableScoresMax) {
    d_variableScoresMax = score;
  }

  // Enqueue the variable
  enqueue(var);
}

bool VariablePriorityQueue::inQueue(Variable var) const {
  Assert(var.typeIndex() < d_variableScores.size());
  Assert(var.index() < d_variableScores[var.typeIndex()].size());
  return d_variableQueuePositions[var.typeIndex()][var.index()] != variable_queue::point_iterator();
}

void VariablePriorityQueue::enqueue(Variable var) {
  Assert(var.typeIndex() < d_variableScores.size());
  Assert(var.index() < d_variableScores[var.typeIndex()].size());
  // Add to the queue
  variable_queue::point_iterator it = d_variableQueue.push(var);
  d_variableQueuePositions[var.typeIndex()][var.index()] = it;
}


void VariablePriorityQueue::bumpVariable(Variable var) {
  Assert(var.typeIndex() < d_variableScores.size());
  Assert(var.index() < d_variableScores[var.typeIndex()].size());
  // New heuristic value
  double newValue = d_variableScores[var.typeIndex()][var.index()] + d_variableScoreIncreasePerBump;
  if (newValue > d_variableScoresMax) {
    d_variableScoresMax = newValue;
  }

  if (inQueue(var)) {
    // If the variable is in the queue, erase it first
    d_variableQueue.erase(d_variableQueuePositions[var.typeIndex()][var.index()]);
    d_variableScores[var.typeIndex()][var.index()] = newValue;
    enqueue(var);
  } else {
    // Otherwise just update the value
    d_variableScores[var.typeIndex()][var.index()] = newValue;
  }

  // If the new value is too big, update all the values
  if (newValue > d_maxScoreBeforeScaling) {
    // This preserves the order, we're fine
    for (unsigned type = 0; type < d_variableScores.size(); ++ type) {
      for (unsigned i = 0; i < d_variableScores[type].size(); ++ i) {
        d_variableScores[type][i] *= d_maxScoreBeforeScaling;
      }
    }
    // TODO: decay activities
    // variableHeuristicIncrease *= 1e-100;
    d_variableScoresMax *= d_maxScoreBeforeScaling;
  }
}

double VariablePriorityQueue::getScore(Variable var) const {
  Assert(var.typeIndex() < d_variableScores.size());
  Assert(var.index() < d_variableScores[var.typeIndex()].size());
  return d_variableScores[var.typeIndex()][var.index()];
}

Variable VariablePriorityQueue::pop() {
  Variable var = d_variableQueue.top();
  d_variableQueue.pop();
  d_variableQueuePositions[var.typeIndex()][var.index()] = variable_queue::point_iterator();
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

  for (unsigned type = 0; type < d_variableScores.size(); ++ type) {
    if (d_variableScores[type].size() > 0) {
      VariableGCInfo::const_iterator it = vReloc.begin(type);
      VariableGCInfo::const_iterator it_end = vReloc.begin(type);

      for (; it != it_end; ++ it) {
        Variable erased = *it;
        if (inQueue(erased)) {
          d_variableQueue.erase(d_variableQueuePositions[erased.typeIndex()][erased.index()]);
          d_variableQueuePositions[erased.typeIndex()][erased.index()] = variable_queue::point_iterator();
        }
        d_variableScores[erased.typeIndex()][erased.index()] = 0;
      }
    }
  }
}
