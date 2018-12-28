// RUN: %check %s

f(const int *p, volatile struct A *pa)
{
	int *q = p; // CHECK: /warning: mismatching types/
	struct A *pb;

	pb = pa; // CHECK: /warning: mismatching types/

	return *q;
}
