// RUN: %check --only -e %s -Werror=incompatible-pointer-types -Werror=incompatible-pointer-types -Wno-cast-qual
//

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

	return f(
			(const struct A *)0, // CHECK: error: mismatching types, argument 1 to f
			(int *)0); // CHECK: error: mismatching types, argument 2 to f
}
