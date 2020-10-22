// RUN: %check -e %s
struct A x[] = {{{{}}}}; // CHECK: error: initialising incomplete type 'struct A[]'
