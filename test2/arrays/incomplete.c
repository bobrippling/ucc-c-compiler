// RUN: %check -e %s

int f(struct incompl x[]); // CHECK: error: array has incomplete type 'struct incompl'
