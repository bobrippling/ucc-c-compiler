// RUN: %check -e %s
struct A
{
	int i, j;
};

f()
{
	struct A; // CHECK: /note: forward/
	struct A a;

	return a.i; // CHECK: /error: .*incomplete/
}
