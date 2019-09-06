// RUN: %ucc -fsyntax-only %s
// RUN: %check -e %s -DERRORS

_Static_assert(_Generic(1e+2, double: 0) == 0);
_Static_assert(_Generic(01e+2, double: 0) == 0);

#ifdef ERRORS
int a = 0b1e+2; // CHECK: error: invalid suffix on integer constant (e)

int b = 0x1e+2;
// this should be 'invalid suffix on integer constant', but this kind of number tokenisation depends on the preprocessor
// (see test/cases/cpp/number_tokenisation.c)
#endif
