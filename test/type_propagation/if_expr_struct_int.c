// RUN: %check -e %s

struct A
{
	double x, y;
} a;

struct A f(int cond)
{
	cond ? a : 0; // CHECK: error: conditional type mismatch (struct A vs int)
}
