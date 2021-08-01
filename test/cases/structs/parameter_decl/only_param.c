// RUN: %check %s

f(struct A *); // CHECK: /warning: declaration of 'struct A' only visible inside function/
