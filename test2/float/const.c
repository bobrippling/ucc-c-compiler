// RUN: %ucc -fsyntax-only %s
// RUN: %check %s

int a[(int)2.3]; // CHECK: !/warning:.*standard/
