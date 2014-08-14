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

always_inline void addr(int x)
{
	int *p = &x;
	*p = 3;
}

always_inline void write(int x)
{
	x++;
}

always_inline void rec(int depth)
{
	if(depth < 5)
		rec(depth + 1); // CHECK: can't always_inline function: can't see function
}

always_inline int should_inline()
{
	return 3;
}

main()
{
	should_inline(); // CHECK: !/warn|error/

	rec(0); // CHECK: !/warn|error/

	hidden(3); // CHECK: error: can't always_inline function: can't see function
	noinline(); // CHECK: error: can't always_inline noinline function
	print("hi", 3); // CHECK: error: can't always_inline function: variadic function
	print("hi"); // CHECK: error: can't always_inline function: variadic function
	old(3, 1); // CHECK: error: can't always_inline function: unspecified argument count function
	addr(5); // CHECK: error: can't always_inline function: argument written or addressed
	write(2); // CHECK: error: can't always_inline function: argument written or addressed
}
