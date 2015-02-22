// RUN: %check --prefix=A -e %s -DA
// RUN: %check --prefix=B -e %s -DB
// RUN: %check --prefix=C -e %s -DC
// RUN: %check --prefix=D -e %s -DD

#ifdef A
float e = 12.3e-4.5; // CHECK-A: error: expecting token ';', got float-literal (parse_type.c:2090)
#endif

#ifdef B
float h = 03e0x7; // CHECK-B: error: invalid suffix on integer constant (x)
#endif

#ifdef C
float j = 3le2; // CHECK-C: error: invalid suffix on integer constant (e)
#endif

#ifdef D
float m = 1ef; // CHECK-D: error: no digits in exponent
#endif
