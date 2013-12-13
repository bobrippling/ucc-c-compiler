// RUN: %check -e %s

struct A
{
	int i;
	struct
	{
		const int k;
	} yo;
};

struct B
{
	const struct
	{
		int xyz;
	} abc;
	int i;
};

f(struct A *p)
{
	struct A x;
	x = *p; // CHECK: /error: can't modify const expression/
}

g(struct B *p)
{
	struct B x;
	x = *p; // CHECK-todo: /error: can't modify const expression/
	// ^this won't fire because we exit on the first error currently
}
