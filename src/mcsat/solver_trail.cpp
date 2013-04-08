#include "mcsat/solver_trail.h"
#include "mcsat/clause/clause_db.h"
#include "mcsat/variable/variable_db.h"

using namespace CVC4;
using namespace CVC4::mcsat;

SolverTrail::SolverTrail(context::Context* context)
: d_decisionLevel(0)
, d_modelValues(context)
, d_context(context)
{
  c_TRUE = NodeManager::currentNM()->mkConst<bool>(true);
  c_FALSE = NodeManager::currentNM()->mkConst<bool>(false);

  Variable varTrue = VariableDatabase::getCurrentDB()->getVariable(c_TRUE);
  Variable varFalse = VariableDatabase::getCurrentDB()->getVariable(c_FALSE);

  // Propagate out TRUE and !FALSE
  PropagationToken out(*this, PropagationToken::PROPAGATION_INIT);
  Literal l1(varTrue, false);
  Literal l2(varFalse, true);
  out(l1);
  out(l2);
}

SolverTrail::~SolverTrail() {
}

void SolverTrail::newDecision() {
  Assert(consistent());

  d_context->push();
  d_decisionLevel ++;
  d_decisionTrail.push_back(d_trail.size());
}

void SolverTrail::popDecision(std::vector<Variable>& variablesUnset) {

  Assert(d_decisionTrail.size() > 0);

  // Pop the trail elements
  while (d_trail.size() > d_decisionTrail.back()) {

    // The variable that was set
    Variable var = d_trail.back().var;
    variablesUnset.push_back(var);

    // Unset all the variable info
    d_model[var] = Node::null();
    d_modelInfo[var].decisionLevel = 0;
    d_modelInfo[var].trailIndex = 0;

    // Remember the reason
    Literal var_pos(var, false);
    Literal var_neg(var, true);

    // Remove any reasons
    d_clauseReasons[var_pos] = CRef_Strong::null;
    d_clauseReasons[var_neg] = CRef_Strong::null;
    d_reasonProviders[var_pos] = 0;
    d_reasonProviders[var_neg] = 0;

    // Pop the element
    d_trail.pop_back();
  }

  // Update the info
  d_decisionTrail.pop_back();
  d_decisionLevel --;
  d_context->pop();

  // If we were inconsistent, not anymore
  d_inconsistentPropagations.clear();
}

void SolverTrail::popToLevel(unsigned level, std::vector<Variable>& variablesUnset) {
  Debug("mcsat::trail") << "SolverTrail::popToLevel(" << level << "): at level " << d_decisionLevel << std::endl;
  while (d_decisionLevel > level) {
    popDecision(variablesUnset);
  }
  Debug("mcsat::trail") << "SolverTrail::popToLevel(" << level << "): new trail: " << *this << std::endl;
}

void SolverTrail::addNewClauseListener(ClauseDatabase::INewClauseNotify* listener) const {
  for (unsigned i = 0; i < d_clauses.size(); ++ i) {
    d_clauses[i]->addNewClauseListener(listener);
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

  Assert(!d_trail.isFalse(l));
  Assert(true); // TODO: Check that l evaluates to true in the model

  if (!d_trail.isTrue(l)) {
    if (l.isNegated()) {
      d_trail.setValue(l.getVariable(), d_trail.c_FALSE, false);
    } else {
      d_trail.setValue(l.getVariable(), d_trail.c_TRUE, false);
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

  Assert(true); // TODO: Check that reason propagates l

  // If new propagation, record in model and in trail
  if (!d_trail.isTrue(l)) {
    // Check that we're not in conflict with this propagation
    if (d_trail.isFalse(l)) {
      // Conflict
      Debug("mcsat::trail") << "PropagationToken::operator () (" << l << ", " << reason << "): conflict" << std::endl;
      d_trail.d_inconsistentPropagations.push_back(InconsistentPropagation(l, reason));
    } else {
      // No conflict, remember the l value in the model
      if (l.isNegated()) {
	d_trail.setValue(l.getVariable(), d_trail.c_FALSE, false);
      } else {
	d_trail.setValue(l.getVariable(), d_trail.c_TRUE, false);
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
  Assert(d_trail.consistent());
  Assert(d_trail.d_model[l.getVariable()].isNull());

  Debug("mcsat::trail") << "DecisionToken::operator () (" << l << ")" << std::endl;

  d_trail.newDecision();
  d_used = true;

  if (l.isNegated()) {
    d_trail.setValue(l.getVariable(), d_trail.c_FALSE, false);
  } else {
    d_trail.setValue(l.getVariable(), d_trail.c_TRUE, false);
  }
  // Set the model information
  Variable var = l.getVariable();
  d_trail.d_modelInfo[var].decisionLevel = d_trail.decisionLevel();
  d_trail.d_modelInfo[var].trailIndex = d_trail.d_trail.size();
  // Push to the trail
  d_trail.d_trail.push_back(Element(BOOLEAN_DECISION, l.getVariable()));
}

void SolverTrail::DecisionToken::operator () (Variable var, TNode val, bool track) {
  Assert(!d_used);
  Assert(d_trail.consistent());
  Assert(d_trail.d_model[var].isNull());

  Debug("mcsat::trail") << "DecisionToken::operator () (" << var << ", " << val << ")" << std::endl;

  d_trail.newDecision();
  d_used = true;

  d_trail.setValue(var, val, track);

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
