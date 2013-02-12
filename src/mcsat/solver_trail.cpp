#include "mcsat/solver_trail.h"
#include "mcsat/clause/clause_db.h"
#include "mcsat/variable/variable_db.h"

using namespace CVC4;
using namespace CVC4::mcsat;

SolverTrail::SolverTrail(const ClauseDatabase& clauses, context::Context* context)
: d_decisionLevel(0)
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
    Variable var = l.getVariable();
    d_trail.d_modelInfo[var].decisionLevel = d_trail.decisionLevel();
    d_trail.d_modelInfo[var].trailIndex = d_trail.d_trail.size();
    d_trail.d_trail.push_back(Element(SEMANTIC_PROPAGATION, var));
  }
}

void SolverTrail::PropagationToken::operator () (Literal l, CRef reason) {

  Debug("mcsat::trail") << "PropagationToken::operator () (" << l << ", " << reason << ")" << std::endl;

  d_used = true;

  Variable currentValue = d_trail.value(l);

  Assert(true); // TODO: Check that reason propagates l

  // If new propagation, record in model and in trail
  if (currentValue != d_trail.c_TRUE) {
    // Check that we're not in conflict with this propagation
    if (currentValue == d_trail.c_FALSE) {
      // Conflict
      d_trail.d_inconsistentPropagations.push_back(InconsistentPropagation(l, reason));
    } else {
      // No conflict, remember the l value in the model
      if (l.isNegated()) {
	d_trail.setValue(l.getVariable(), d_trail.c_FALSE);
      } else {
	d_trail.setValue(l.getVariable(), d_trail.c_TRUE);
      }
      // Set the model information
      Variable var = l.getVariable();
      d_trail.d_modelInfo[var].decisionLevel = d_trail.decisionLevel();
      d_trail.d_modelInfo[var].trailIndex = d_trail.d_trail.size();
      // Remember the reason
      d_trail.d_clauseReasons[l] = reason;
      d_trail.d_reasonProviders[l] = &d_trail.d_clauseReasons;
      // Push to the trail
      d_trail.d_trail.push_back(Element(CLAUSAL_PROPAGATION, var));
    }
  } 
}

void SolverTrail::DecisionToken::operator () (Literal l) {
  Assert(!d_used);
  Assert(d_trail.d_model[l.getVariable()].isNull());

  Debug("mcsat::trail") << "DecisionToken::operator () (" << l << ")" << std::endl;

  d_trail.newDecision();
  d_used = true;

  if (l.isNegated()) {
    d_trail.setValue(l.getVariable(), d_trail.c_FALSE);
  } else {
    d_trail.setValue(l.getVariable(), d_trail.c_TRUE);
  }
  // Set the model information
  Variable var = l.getVariable();
  d_trail.d_modelInfo[var].decisionLevel = d_trail.decisionLevel();
  d_trail.d_modelInfo[var].trailIndex = d_trail.d_trail.size();
  // Push to the trail
  d_trail.d_trail.push_back(Element(BOOLEAN_DECISION, l.getVariable()));
}

void SolverTrail::DecisionToken::operator () (Variable var, Variable val) {
  Assert(!d_used);
  Assert(d_trail.d_model[var].isNull());

  Debug("mcsat::trail") << "DecisionToken::operator () (" << var << ", " << val << ")" << std::endl;

  d_trail.newDecision();
  d_used = true;

  d_trail.setValue(var, val);

  d_trail.d_trail.push_back(Element(SEMANTIC_DECISION, var));
}

bool SolverTrail::hasReason(Literal literal) const {
  // You can resolve with l if ~l has been propagated true with a clausal reason
  return d_reasonProviders[literal] != 0;
}

CRef SolverTrail::reason(Literal literal) {
  Assert(hasReason(literal));
  ReasonProvider* reasonProvider = d_reasonProviders[literal];
  CRef reason = reasonProvider->explain(literal);
  // If not a clausal reason provider, remember just in case
  if (reasonProvider != &d_clauseReasons) {
    d_clauseReasons[literal] = reason;
    d_reasonProviders[literal] = &d_clauseReasons;
  }
  // Return the explanation
  return reason;
}

void SolverTrail::getInconsistentPropagations(std::vector<InconsistentPropagation>& out) const {
  out.clear();
  for (unsigned i = 0; i < d_inconsistentPropagations.size(); ++ i) {
    out.push_back(d_inconsistentPropagations[i]);
  }
}

void SolverTrail::toStream(std::ostream& out) const {
  out << "[";
  for (unsigned i = 0; i < d_trail.size(); ++ i) {
    if (i > 0) {
      out << ", ";
    }
    out << "(" << d_trail[i].var << " -> " << value(d_trail[i].var) << ")";
  }
  out << "]";
}
