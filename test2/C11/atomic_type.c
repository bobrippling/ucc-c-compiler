// RUN: %ucc -fsyntax-only %s

_Atomic(int *) p, q;
_Atomic int *r, s;

/* add a level of indirection to prevent top-level-qual stripping */
#define TY_EQ(e, t) \
_Static_assert(_Generic(&(e), t *: 1) == 1, "")

TY_EQ(p, int *_Atomic);
TY_EQ(q, int *_Atomic);
TY_EQ(r, int _Atomic *);
TY_EQ(s, int _Atomic);
