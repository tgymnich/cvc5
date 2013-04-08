#include "mcsat/rules/fourier_motzkin_rule.h"

using namespace CVC4;
using namespace mcsat;
using namespace rules;
using namespace fm;

FourierMotzkinRule::FourierMotzkinRule(ClauseDatabase& clauseDB)
: ProofRule("mcsat::fourier_motzkin_rule", clauseDB)
{
}

/** Start the resolution */
void FourierMotzkinRule::start(Literal lit) {
  d_assumptions.clear();
  d_resolvent.clear();

  d_assumptions.insert(lit);
  bool linear = LinearConstraint::parse(lit, d_resolvent);

  Debug("mcsat::fm") << "ForuierMotzkinRule: starting with " << d_resolvent << std::endl;

  Assert(linear);
}

/** Resolve with given inequality over the given variable. */
void FourierMotzkinRule::resolve(Variable var, Literal ineq) {

  LinearConstraint toResolve;
  bool linear = LinearConstraint::parse(ineq, toResolve);
  Assert(linear);

  Debug("mcsat::fm") << "ForuierMotzkinRule: resolving " << d_resolvent << std::endl;

  // We know that both are one of >, >= so coeficients with var must be opposite
  Rational d_resolventC = d_resolvent.getCoefficient(var);
  Rational toResolveC = toResolve.getCoefficient(var);
  Assert(d_resolventC*toResolveC < 0);

  // Compute the new resolvent
  d_resolvent.multiply(toResolveC.abs());
  d_resolvent.add(toResolve, d_resolventC.abs());

  // Add to assumptions
  d_assumptions.insert(ineq);

  Debug("mcsat::fm") << "ForuierMotzkinRule: got " << d_resolvent << std::endl;
}

/** Finish the derivation */
CRef FourierMotzkinRule::finish() {
  
  LiteralVector lits;
  
  // Add the lemma assumptions => resolvent
  // (!a1 \vee !a2 \vee ... \veee !an \vee resolvent)
  std::set<Literal>::const_iterator it = d_assumptions.begin();
  std::set<Literal>::const_iterator it_end = d_assumptions.end();
  for (; it != it_end; ++ it) {
    // negation of the assumption
    lits.push_back(~(*it));
  }
  lits.push_back(d_resolvent.getLiteral());
  
  return commit(lits);
}


