// RUN: %check -e %s
struct A x[] = {{{{}}}}; // CHECK: error: initialising struct A[]
