_Atomic(int) i;
_Atomic(int) *p = &i;

_Atomic(_Atomic(int) *) a;
_Atomic int *_Atomic a;

_Atomic(_Atomic(int) *) *b[] = { &a };

_Atomic(int()) e1;        // error
_Atomic(struct A) e2;     // error
_Atomic(int[10]) e3;      // error
_Atomic(const int) e4;    // error
_Atomic(_Atomic(int)) e5; // error
