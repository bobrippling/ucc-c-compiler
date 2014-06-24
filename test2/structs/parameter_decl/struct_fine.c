// RUN: %check %s

struct A;

f(struct A *);

test1()
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

test2()
{
	struct A a;
	f(&a); // CHECK: !/warning/
}
