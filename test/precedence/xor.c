// RUN: %check %s

f(a, b)
{
	a != 1 ^ b; // CHECK: warning: != has higher precedence than ^
}
