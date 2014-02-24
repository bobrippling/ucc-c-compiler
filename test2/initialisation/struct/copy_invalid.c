// RUN: %check -e %s

struct A
{
	int i, j;
};

void g(struct A *p);

void f(struct A *p)
{
	struct A a = *p; // CHECK: !/error/

	g(&(struct A){*p}); // CHECK: error: mismatching types, initialisation:
                      // CHECK:^ note: 'int' vs 'struct A'
}
