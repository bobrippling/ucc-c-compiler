// RUN: %check -e %s -finline-functions

__attribute((always_inline))
int f(int x)
{
	// this being static prevents inlining
	static void *jmps[] = {
		&&a, &&b, &&c
	};

	goto *jmps[x];
b:
	return x + 3;
c:
	return x + 1;
a:
	x += 2;
	goto *&&b;
}

assert(int c)
{
	if(!c){
		extern void abort(void);
		abort();
	}
}

main()
{
	assert(f(0) == 5); // CHECK: error: couldn't always_inline call: function contains static-address-of-label expression
	assert(f(1) == 4); // CHECK: error: couldn't always_inline call: function contains static-address-of-label expression
	assert(f(2) == 3); // CHECK: error: couldn't always_inline call: function contains static-address-of-label expression

	return 0;
}
