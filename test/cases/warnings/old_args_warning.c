// RUN: %check %s

f(a, b, c)
	int a, b, c; // CHECK: !/warn.*non-static declaration has no previous extern declaration/
{
}
