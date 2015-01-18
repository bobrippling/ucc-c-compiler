// RUN: %ucc -fsyntax-only %s

_Atomic(int *) const x, p;
_Atomic(const int *) y;

// redeclare to get type-compat checks
int *const _Atomic x;
int *const _Atomic p;
const int *_Atomic y;
