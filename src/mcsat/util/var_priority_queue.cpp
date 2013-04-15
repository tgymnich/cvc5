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

  // Insert a new score (max of the current scores)
  d_variableScores.resize(var.index() + 1, score);
  // Make sure that there is enough space for the pointer
  d_variableQueuePositions.resize(var.index() + 1);

  if (score > d_variableScoresMax) {
    d_variableScoresMax = score;
  }

  // Enqueue the variable
  enqueue(var);
}

bool VariablePriorityQueue::inQueue(Variable var) const {
  return d_variableQueuePositions[var.index()] != variable_queue::point_iterator();
}

void VariablePriorityQueue::enqueue(Variable var) {
  // Add to the queue
  variable_queue::point_iterator it = d_variableQueue.push(var);
  d_variableQueuePositions[var.index()] = it;
}


void VariablePriorityQueue::bumpVariable(Variable var) {

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

void VariablePriorityQueue::gcRelocate(const VariableRelocationInfo& vReloc) {

  std::vector<Variable> relocatedVariables;
  std::vector<double> relocatedVariablesScores;

  // We only care about the scores of the variables that are in the queue (this is 0-level)
  while (!empty()) {
    Variable oldVar = pop();
    relocatedVariables.push_back(vReloc.relocate(oldVar));
    relocatedVariablesScores.push_back(d_variableScores[oldVar.index()]);
  }

  d_variableScores.clear();
  d_variableScoresMax = 0;
  d_variableQueuePositions.clear();

  for (unsigned i = 0; i < relocatedVariables.size(); ++ i) {
    newVariable(relocatedVariables[i], relocatedVariablesScores[i]);
  }

}
