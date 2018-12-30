// RUN: %check %s

// make sure we don't drop the hello attribute

__attribute((hello)) struct A { int i; } a; // CHECK: warning: ignoring unrecognised attribute "hello"
