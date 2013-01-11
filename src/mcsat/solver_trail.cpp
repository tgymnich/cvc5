#include "mcsat/solver_trail.h"
#include "mcsat/clause/clause_db.h"
#include "mcsat/variable/variable_db.h"

using namespace CVC4;
using namespace CVC4::mcsat;

SolverTrail::SolverTrail(const ClauseDatabase& clauses, context::Context* context)
: d_consistent(true)
, d_decisionLevel(0)
, d_problemClauses(clauses)
, d_auxilaryClauses(ClauseFarm::getCurrent()->newClauseDB("analysis_clauses"))
, d_context(context)
{
  Node trueNode = NodeManager::currentNM()->mkConst<bool>(true);
  Node falseNode = NodeManager::currentNM()->mkConst<bool>(false);

  c_TRUE = VariableDatabase::getCurrentDB()->getVariable(trueNode);
  c_FALSE = VariableDatabase::getCurrentDB()->getVariable(falseNode);

  // Add true and (not false) to the trail
  PropagationToken prop(*this, PropagationToken::PROPAGATION_NORMAL);
  prop(Literal(c_TRUE, false));
  prop(Literal(c_FALSE, true));
}

SolverTrail::~SolverTrail() {
}

void SolverTrail::newDecision() {
  d_context->push();
  d_decisionLevel ++;
  d_decisionTrail.push_back(d_trail.size());
}

void SolverTrail::addNewClauseListener(ClauseDatabase::INewClauseNotify* listener) const {
  d_problemClauses.addNewClauseListener(listener);
  d_auxilaryClauses.addNewClauseListener(listener);
}

void SolverTrail::ClauseReasonProvider::explain(Literal l, std::vector<Literal>& reason) {
  Assert(!d_reasons[l].isNull());
  Clause& clause = d_reasons[l].getClause();
  Assert(clause[0] == l);
  for (unsigned i = 1; i < clause.size(); ++ i) {
    reason.push_back(clause[i]);
  }
}

unsigned SolverTrail::decisionLevel(Variable var) const {
  Assert(!value(var).isNull());
  return d_modelInfo[var].decisionLevel;
}

unsigned SolverTrail::trailIndex(Variable var) const {
  Assert(!value(var).isNull());
  return d_modelInfo[var].trailIndex;
}

void SolverTrail::PropagationToken::operator () (Literal l) {

  Debug("mcsat::trail") << "PropagationToken::operator () (" << l << ")" << std::endl;

  d_used = true;

  Variable currentValue = d_trail.value(l);

  Assert(currentValue != d_trail.c_FALSE);
  Assert(true); // TODO: Check that l evaluates to true in the model

  if (currentValue != d_trail.c_TRUE) {
    if (l.isNegated()) {
      d_trail.setValue(l.getVariable(), d_trail.c_FALSE);
    } else {
      d_trail.setValue(l.getVariable(), d_trail.c_TRUE);
    }
    d_trail.d_trail.push_back(Element(SEMANTIC_PROPAGATION, l.getVariable()));
  }
}

void SolverTrail::PropagationToken::operator () (Literal l, CRef reason) {

  Debug("mcsat::trail") << "PropagationToken::operator () (" << l << ", " << reason << ")" << std::endl;

  d_used = true;

  Variable currentValue = d_trail.value(l);

  Assert(true); // TODO: Check that reason propagates l

  // If new propagation, record in model and in trail
  if (currentValue.isNull()) {
    if (l.isNegated()) {
      d_trail.setValue(l.getVariable(), d_trail.c_FALSE);
    } else {
      d_trail.setValue(l.getVariable(), d_trail.c_TRUE);
    }
    d_trail.d_trail.push_back(Element(SEMANTIC_PROPAGATION, l.getVariable()));
    d_trail.d_clauseReasons[l] = reason;
  }
}

void SolverTrail::DecisionToken::operator () (Literal l) {
  Assert(!d_used);
  Assert(d_trail.d_model[l.getVariable()].isNull());

  Debug("mcsat::trail") << "PropagationToken::operator () (" << l << ")" << std::endl;

  d_trail.newDecision();
  d_used = true;

  if (l.isNegated()) {
    d_trail.setValue(l.getVariable(), d_trail.c_FALSE);
  } else {
    d_trail.setValue(l.getVariable(), d_trail.c_TRUE);
  }
  d_trail.d_trail.push_back(Element(BOOLEAN_DECISION, l.getVariable()));
}

void SolverTrail::DecisionToken::operator () (Variable var, Variable val) {
  Assert(!d_used);
  Assert(d_trail.d_model[var].isNull());

  Debug("mcsat::trail") << "PropagationToken::operator () (" << var << ", " << val << ")" << std::endl;

  d_trail.newDecision();
  d_used = true;

  d_trail.setValue(var, val);

  d_trail.d_trail.push_back(Element(SEMANTIC_DECISION, var));
}
