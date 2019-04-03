// RUN: %check --only %s

struct A;

f(struct A *);

void test1()
{
	struct A
	{
		int i, j;
	} a;

	f(&a); // CHECK: /warning: mismatching types, argument/
}

struct A
{
	int i, j;
};

void test2()
{
	struct A a;
	f(&a); // CHECK: !/warning/
}
