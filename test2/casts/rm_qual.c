// RUN: %check %s

struct A
{
	int i;
};

int f(const void *p)
{
	struct A *a = p; // CHECK: warning: implicit cast removes qualifiers (const)
	struct A *b = (struct A *)p; // CHECK: !/warning:.*cast removes qualifiers/

	(void)a;
	(void)b;
}
