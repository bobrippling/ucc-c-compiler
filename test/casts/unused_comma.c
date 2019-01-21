// RUN: %check %s

f(int i, int j, int *x)
{
	x[      i, j] = 5; // CHECK: /warning: left hand side of comma is unused/
	x[(void)i, j] = 5; // CHECK: !/warn/
}
