// RUN: %check -e %s

struct A
{
	int i;
};

f(int i, struct A *p)
{
	struct A b;

	(i ? p : (void *)&b)->i; // CHECK: error: 'void *' (if-expr) is not a pointer to struct or union (member i)
}
