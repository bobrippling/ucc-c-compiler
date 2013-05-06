// RUN: %ucc -E -o %t %s
// RUN: grep 'int x;' %t > /dev/null
// RUN: grep 'x \+x' %t > /dev/null

int x; // comment

#define A(foo, bar) foo bar
#define B x // comment

A(B, B)
