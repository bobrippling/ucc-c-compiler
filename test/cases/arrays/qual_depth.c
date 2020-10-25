// RUN: %ucc -fsyntax-only %s

typedef int A[2][3];

// this tests a bug with type_is_vla() and init
volatile A a = {{4},{7}};

// this ensures volatile gets passed down through all levels of the array type
x = _Generic(a, int volatile [2][3]: 1);
