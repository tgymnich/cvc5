(set-logic QF_LRA)
(set-info :smt-lib-version 2.0)
(declare-fun a0 () Real)
(declare-fun b0 () Real)
(declare-fun c0 () Real)
(declare-fun a1 () Real)
(declare-fun b1 () Real)
(declare-fun c1 () Real)
(declare-fun a2 () Real)
(declare-fun b2 () Real)
(declare-fun c2 () Real)
(declare-fun a3 () Real)
(declare-fun b3 () Real)
(declare-fun c3 () Real)
(declare-fun a4 () Real)
(declare-fun b4 () Real)
(declare-fun c4 () Real)
(declare-fun a5 () Real)
(declare-fun b5 () Real)
(declare-fun c5 () Real)
(assert (or (and (< a0 b0) (< b0 a1)) (and (< a0 c0) (< c0 a1))))
(assert (or (and (< a1 b1) (< b1 a2)) (and (< a1 c1) (< c1 a2))))
(assert (or (and (< a2 b2) (< b2 a3)) (and (< a2 c2) (< c2 a3))))
(assert (or (and (< a3 b3) (< b3 a4)) (and (< a3 c3) (< c3 a4))))
(assert (or (and (< a4 b4) (< b4 a5)) (and (< a4 c4) (< c4 a5))))
(assert (> a0 a5))
(check-sat)
(exit)