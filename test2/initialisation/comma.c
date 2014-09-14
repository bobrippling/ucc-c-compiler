// RUN: %check %s -pedantic
int x[2];
int *p = &x[1, 0]; // CHECK: /warning: global scalar initialiser contains non-standard constant expression/
