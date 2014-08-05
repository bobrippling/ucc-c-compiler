// RUN: %check -e %s

f(int (*pa)[2])
{
	int x[2];
	*pa = x; // CHECK: error: assignment to int[2] - not an lvalue
}
