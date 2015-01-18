// RUN: %ucc -fsyntax-only %s

_Atomic int i;

_Atomic const int  j;
_Atomic _Atomic int k;
_Atomic _Atomic(int) l;
