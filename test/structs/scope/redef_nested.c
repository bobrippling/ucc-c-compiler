// RUN: %check -e %s

struct A; // CHECK: note: forward declared here

int f(struct A *p)
{
	struct A // different type
	{
		int x;
	} a;

	p = &a; // CHECK: warning: mismatching types, assignment

	return p->x; // CHECK: error: dereferencing pointer to incomplete type (struct A *)
}
