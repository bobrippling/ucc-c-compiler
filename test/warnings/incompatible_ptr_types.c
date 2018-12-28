// RUN: %check -e %s -Werror=incompatible-pointer-types

struct A
{
	int i;
};

int f(struct A *p, const struct A *q)
{
	p = q; // CHECK: error: mismatching types, assignment
	return 3;
}

int g()
{
	int *p = (struct A *)0; // CHECK: error: mismatching types, initialisation

	f(
			(const struct A *)0, // CHECK: error: mismatching types, argument 1 to f
			(int *)0); // CHECK: error: mismatching types, argument 2 to f
}
