// RUN: %check -e %s

#define always_inline __attribute((always_inline))

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

main()
{
	should_inline(); // CHECK: !/warn|error/

	rec(0); // CHECK: !/warn|error/

	hidden(3); // CHECK: error: couldn't always_inline call: can't see function code
	noinline(); // CHECK: error: couldn't always_inline call: function has noinline attribute
	print("hi", 3); // CHECK: error: couldn't always_inline call: call to variadic function
	print("hi"); // CHECK: error: couldn't always_inline call: call to variadic function
	old(3, 1); // CHECK: error: couldn't always_inline call: call to function with unspecified arguments
}
