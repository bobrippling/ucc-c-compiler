/* An atomic object with automatic storage duration that is not explicitly
 * initialized using ATOMIC_VAR_INIT is initially in an indeterminate state;
 * however, the default (zero) initialization for objects with static or
 * thread-local storage duration is guaranteed to produce a valid state that
 * corresponds to the value of a zero initialized object of the unqualified
 * base type. */

/* EXAMPLE All three of the following objects initially have an observable
 * value of 0. */
_Atomic(unsigned) A = { 0 };
_Atomic(unsigned) B = ATOMIC_VAR_INIT(0u);
static _Atomic(unsigned) C;
