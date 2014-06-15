// RUN: %check -e %s

f(int x, char *y, char buf[static (__typeof(y))sizeof x]);
// CHECK: ^ error: array type isn't integral
