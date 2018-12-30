// RUN: %check %s

struct A { int : 3; }; // CHECK: warning: struct has no named members
struct B {}; // CHECK: warning: struct is empty
