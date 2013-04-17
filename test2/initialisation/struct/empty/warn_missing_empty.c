// RUN: %layout_check %s

struct A {};
struct B { int i; struct A a; int j; } x = { 1, 2 };
