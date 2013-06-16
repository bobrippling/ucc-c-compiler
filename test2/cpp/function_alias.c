// RUN: %check -e %s

#define F(a, b, c) a + b - c
#define A F
A() // CHECK: /error: wrong number of args to function macro "F", got 0, expected 3/
