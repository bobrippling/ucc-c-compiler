// RUN: %check -e %s

// this mostly checks we don't segfault on a null .sym in the expr's identifier bits

_Static_assert(sizeof(struct A { int *p; }){ &not_declared }.p == 3, ""); // CHECK: error: undeclared identifier "not_declared"
