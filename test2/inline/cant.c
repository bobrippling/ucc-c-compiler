// RUN: %check -e %s -fno-semantic-interposition

#define always_inline __attribute((always_inline))

// necessary as -fno-semantic-interposition doesn't affect extern decls:
__attribute((visibility("hidden")))
always_inline hidden(int);

void always_inline __attribute((noinline)) noinline(void)
{
}

always_inline void print(const char *fmt, ...)
{
}

always_inline void old(a, b)
	int a, b;
{
}

always_inline void rec(int depth)
{
	if(depth < 5)
		rec(depth + 1); // CHECK: error: couldn't always_inline call: recursion too deep
}

always_inline int should_inline()
{
	return 3;
}

always_inline int computed_goto()
{
	static void *p = &&L;
	// this can't be duplicated because of how const_folds() works
	// - it ignores backend codegen so will always return the exact same
	// (eventually duplicated) label

	goto *p;
L:
	return 5;
}

main()
{
	should_inline(); // CHECK: !/warn|error/

	rec(0); // CHECK: !/warn|error/

	hidden(3); // CHECK: error: couldn't always_inline call: can't see function code
	noinline(); // CHECK: error: couldn't always_inline call: function has noinline attribute
	print("hi", 3); // CHECK: error: couldn't always_inline call: call to variadic function
	print("hi"); // CHECK: error: couldn't always_inline call: call to variadic function
	old(3, 1); // CHECK: error: couldn't always_inline call: call to function with unspecified arguments
	computed_goto(); // CHECK: error: couldn't always_inline call: function contains static-address-of-label expression
}
