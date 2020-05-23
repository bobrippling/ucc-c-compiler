// RUN: %check --only -e %s

struct A
{
	int i;
};

void f(int i, struct A *p)
{
	struct A b;

	(void)(i ? p : (void *)&b)->i; // CHECK: error: 'void *' (if-expr) is not a pointer to struct or union (member i)
}

struct B
{
	double x, y;
} a;

void g(int cond)
{
	(void)(cond ? a : 0); // CHECK: error: conditional type mismatch (struct B vs int)
}
