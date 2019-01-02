// RUN: %check -e %s

static __attribute((always_inline)) inline
void *f()
{
	static int i;
	static int *p = &i;
	return p;
}

static __attribute((always_inline)) inline
void *g()
{
	static void *p = &&a;
a:
	return p;
}

main()
{
	// f() can be inlined - static unary-& is fine
	f(); // CHECK: !/error/
	g(); // CHECK: error: couldn't always_inline call: function contains static-address-of-label expression
}
