// RUN: %check -e %s

struct A;

a = __builtin_offsetof(struct A, a)]; // CHECK: error: offsetof() on incomplete type 'struct A'
