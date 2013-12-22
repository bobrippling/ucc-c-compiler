// RUN: %check -e %s

struct A
{
	int x[2];
};

main()
{
	struct A a;
	int x[2];

	a.x = x; // CHECK: error: assignment to int[2]/struct - not an lvalue
}
