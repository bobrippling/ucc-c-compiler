// RUN: %check %s
f(a, b, c)
{
	int i = a || b && c; // CHECK: /warning: && has higher precedence than \|\|/
	return i && b || c; // CHECK: /warning: && has higher precedence than \|\|/
}
