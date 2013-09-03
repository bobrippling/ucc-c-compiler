// RUN: %check -e %s
struct A
{
	int i, j;
};

f()
{
	struct A; // new struct A
	struct A a;

	return a.i; // CHECK: /error: .*incomplete/
}
