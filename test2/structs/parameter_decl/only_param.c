// RUN: %check %s

f(struct A *); // CHECK: /warning: struct A is declared only for this function/
