; EXPECT: sat
(set-option :incremental false)
(set-logic ALL)

(declare-fun x () (Set (Tuple (_ BitVec 2) (_ BitVec 2))))
(declare-fun y () (Set (Tuple (_ BitVec 2) (_ BitVec 2))))
(declare-fun a () (_ BitVec 2))
(declare-fun b () (_ BitVec 2))
(declare-fun c () (_ BitVec 2))
(declare-fun d () (_ BitVec 2))
(declare-fun e () (_ BitVec 2))
(assert (not (= b c)))
(assert (member (tuple a b) x))
(assert (member (tuple a c) x))
(assert (member (tuple d a) y))
(assert (not (member (tuple a a) (join x y))))
(check-sat)
