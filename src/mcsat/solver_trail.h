#pragma once

#include "cvc4_private.h"

#include <set>
#include <vector>

#include "mcsat/variable/variable_db.h"
#include "mcsat/variable/variable_table.h"
#include "mcsat/clause/clause_db.h"
#include "mcsat/clause/literal_table.h"

#include "context/cdinsert_hashmap.h"

namespace CVC4 {
namespace mcsat {

class SolverPlugin;

/** Possible values a boolean term can have */
enum BooleanValue {
  /** True */
  BOOL_TRUE,
  /** False */
  BOOL_FALSE,
  /** Unknown */
  BOOL_UNKNOWN
};


class SolverTrail {

public:

  /** Types of elements in the trail */
  enum Type {
    /** Decision on value of a Boolean typed variable */
    BOOLEAN_DECISION,
    /** Decision on value of a non-Boolean typed variable */
    SEMANTIC_DECISION,
    /** Propagation of a Boolean typed variable supported by a clause */
    CLAUSAL_PROPAGATION,
    /** Propagation of a Boolean typed variable supported by the model */
    SEMANTIC_PROPAGATION,
  };

  /**
   * The element of the trail only records the variable that became assigned and
   * the type of event that led to its assignment.
   */
  struct Element {
    /** Type of the element */
    Type type;
    /** The variable that got changed */
    Variable var;

    /** Is this a decision */
    bool isDecision() const {
      return type == BOOLEAN_DECISION || type == SEMANTIC_DECISION;
    }

    /** Is it a propagation with a clausal reason */
    bool hasReason() const {
      return type == CLAUSAL_PROPAGATION;
    }

    Element(Type type, Variable var)
    : type(type), var(var) {}
  };

  /**
   * An object that can provide a reason for the propagation
   */
  class ReasonProvider {
  public:
    /**
     * Provide the clausal explanation of the propagation of l.
     */
    virtual CRef explain(Literal l) = 0;
  };

  /** Captures the inconsistently propagated literal and the reason for it */
  struct InconsistentPropagation {
    /** Literal that was propagated */
    Literal literal;
    /** The reason of this propagation */
    CRef reason;

    InconsistentPropagation(Literal literal, CRef reason)
    : literal(literal), reason(reason) {}
  };

  /** True value */
  Variable c_TRUE;

  /** False value */
  Variable c_FALSE;

  /** Print the trail to the stream */
  void toStream(std::ostream& out) const;

private:

  /**
   * Simple reason provider based on a clause.
   */
  class ClauseReasonProvider : public ReasonProvider {
    typedef literal_table<CRef_Strong> reasons_map;

    /** Map from literals to clausal reasons */
    reasons_map d_reasons;
  public:
    CRef explain(Literal l) {
      return d_reasons[l];
    }
    CRef_Strong& operator [] (Literal l) { return d_reasons[l]; }
  };
  
  /** The actual trail */
  std::vector<Element> d_trail;

  /** Reason providers for clausal propagations */
  literal_table<ReasonProvider*> d_reasonProviders;

  /** Vector of propagated literals that are inconsistent */
  std::vector<InconsistentPropagation> d_inconsistentPropagations;

  /** Trail containing trail sizes per decision level */
  std::vector<size_t> d_decisionTrail;
  
  /** Number of decisions */
  size_t d_decisionLevel;
  
  /** Mark a new decision */
  void newDecision();
  
  /** Clause of the problem */
  const ClauseDatabase& d_problemClauses;
  
  /** Auxiliary clause important for the search (conflict analysis clauses) */
  ClauseDatabase& d_auxilaryClauses;

  /** Model indexed by variables */
  variable_table<Variable_Strong> d_model;

  struct VariableInfo {
    unsigned decisionLevel;
    unsigned trailIndex;
  };

  /** Information on the model variables */
  variable_table<VariableInfo> d_modelInfo;

  void setValue(Variable var, Variable value) {
    Assert(d_model[var].isNull());
    d_model[var] = value;
  }

  /** The context of the search */
  context::Context* d_context;

  /** Reason provider for clause propagations */
  ClauseReasonProvider d_clauseReasons;
  
public:
  
  /** Create an empty trail with the give set of clauses */
  SolverTrail(const ClauseDatabase& clauses, context::Context* context);
  
  /** Destructor */
  ~SolverTrail();

  /** Get the i-th element of the trail */
  const Element& operator [] (size_t i) const { 
    return d_trail[i]; 
  }

  /** Get the size of the trail */
  size_t size() const { 
    return d_trail.size();     
  }
  
  /** Get the size of the trail at given decision level */
  size_t size(unsigned level) const {
    if (level >= d_decisionLevel) return d_trail.size();
    else return d_decisionTrail[level];
  }

  /** Is the trail consistent */
  bool consistent() const {
    return d_inconsistentPropagations.empty();
  }

  /** Returns the current decision level */
  size_t decisionLevel() const {
    return d_decisionLevel;
  }
  
  /** Returns the context that the trail is controlling */
  context::Context* getSearchContext() const {
    return d_context;
  }

  /** Return the value of the literal in the current trail */
  Variable value(Literal l) const {
    Variable v = d_model[l.getVariable()];
    if (l.isNegated()) {
      if (v == c_TRUE) {
        return c_FALSE;
      }
      if (v == c_FALSE) {
        return c_TRUE;
      }
    }
    return v;
  }

  /** Returns true if the literal is true in the current model */
  bool isTrue(Literal l) const {
    return value(l) == c_TRUE;
  }

  /** Returns true if the literal is true in the current model */
  bool isFalse(Literal l) const {
    return value(l) == c_FALSE;
  }

  /** Get the decision level where the variable was assigned */
  unsigned decisionLevel(Variable var) const;
  /** Get the trail index where the variable was assigned */
  unsigned trailIndex(Variable var) const;

  /** Returns the reason for this clause */
  CRef reason(Literal literal);
  /** Does this literal have a clausal reason for it's value */
  bool hasReason(Literal literal) const;


  /** Returns inconsistent propagations */
  void getInconsistentPropagations(std::vector<InconsistentPropagation>& out) const;

  /** Get the true constant */
  Variable getTrue() const { return c_TRUE; }
  /** Get the false constant */
  Variable getFalse() const { return c_FALSE; }

  /** Return the value of a Boolean value in the current trail */
  Variable value(Variable var) const {
    return d_model[var];
  }

  /**
   * Add a listener for the creation of new clauses. A context independent listener will only be notified
   * once when the clause is created. If the listener is context dependent, it will be notified when the clause is
   * created, but it will be re-notified on every pop so that it can update it's internal state.
   *
   * @param listener the listener
   * @param contextDependent whether the listener is context dependent
   */
  void addNewClauseListener(ClauseDatabase::INewClauseNotify* listener) const;

  /**
   * Request a backtrack to a given decision level. Each backtrack request must be accompanied with a clause that
   * needs to be satisfied. That means that the any assigned literals in the clause must be false.
   */
  void requestBacktrack(size_t decisionLevel, CRef cref);

  /** Token for modules to perform propagations */
  class PropagationToken {

  public:

    /** Mode of propagation */
    enum Mode {
      /** Initial propagation after new information has been added */
      PROPAGATION_INIT,
      /** Regular propagation */
      PROPAGATION_NORMAL,
      /** Complete propagation, have to be sure */
      PROPAGATION_COMPLETE
    };

  private:

    /** The trail that the token controls */
    SolverTrail& d_trail;

    /** The mode of propagation */
    Mode d_mode;

    /** Has the token been used */
    bool d_used;

  public:

    PropagationToken(SolverTrail& trail, Mode mode)
    : d_trail(trail), d_mode(mode), d_used(false) {}

    /** Get the mode of propagation */
    Mode mode() const { return d_mode; }
    /** Has the token been used */
    bool used() const { return d_used; }

    /**
     * Semantic propagation based on current model in the trail. A literal that is true in the model must evaluate
     * to true. The literal will be marked with the level at which is true, and will be repropagated on backtracks.
     */
    void operator () (Literal l);

    /**
     * Propagation based on clause that propagates under the current Boolean assignment in the trail.
     */
    void operator () (Literal l, CRef reason);

    /** Same as propagation based on a clause, but the clause will be constructed on-demand,  reason */
    void operator () (Literal l, ReasonProvider* reason_provider);
  };
  
  /** Token for modules to perform decisions */
  class DecisionToken {

    /** The trail that the token controls */
    SolverTrail& d_trail;

    /** Has the token been used */
    bool d_used;

  public:
    DecisionToken(SolverTrail& trail)
    : d_trail(trail), d_used(false) {}

    /** Has the token been used */
    bool used() const { return d_used; }

    /** Decide a Boolean typed variable to a value */
    void operator () (Literal lit);
    /** Decide a non-Boolean typed variable to a value */
    void operator () (Variable variable, Variable value);
  };

  friend class PropagationToken;
  friend class DecisionToken;
};

inline std::ostream& operator << (std::ostream& out, const SolverTrail::PropagationToken::Mode& mode) {
  switch (mode) {
  case SolverTrail::PropagationToken::PROPAGATION_INIT:
    out << "PROPAGATION_INIT";
    break;
  case SolverTrail::PropagationToken::PROPAGATION_NORMAL:
    out << "PROPAGATION_NORMAL";
    break;
  case SolverTrail::PropagationToken::PROPAGATION_COMPLETE:
    out << "PROPAGATION_COMPLETE";
    break;
  }
  return out;
}

inline std::ostream& operator << (std::ostream& out, const SolverTrail& trail) {
  trail.toStream(out);
  return out;
}

}
}
