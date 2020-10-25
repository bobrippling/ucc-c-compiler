// RUN: %check %s

struct A
{
	int i;
};

void take(void *);

int f(const void *p)
{
	struct A *a = p; // CHECK: warning: implicit cast removes qualifiers (const)
	struct A *b = (struct A *)p; // CHECK: !/warning:.*cast removes qualifiers/

	(void)a;
	(void)b;

	const char c = 5;
	take(&c); // CHECK: warning: implicit cast removes qualifiers (const)
}
