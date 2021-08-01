// RUN: %check -e %s

struct A { int i; } b = 2; // CHECK: error: struct A must be initialised with an initialiser list
