// RUN: %ucc -E %s | grep -F '3 + 2 + 1'

#define A(x) x + 1
#define B(y) y + 2
A(B(3))
