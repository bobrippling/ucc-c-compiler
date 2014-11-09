// RUN: %check -e %s

int g();

__attribute((always_inline))
void a(int vm[g()][2]) // can inline
{
}

__attribute((always_inline))
void b(int vm[g()][g()]) // can't inline
{
}

__attribute((always_inline))
void c(int vm[2][2]) // can inline
{
}

__attribute((always_inline))
void d(int vm[2][g()]) // can't inline
{
}

main()
{
	a(0); // CHECK: !/error/
	b(0); // CHECK: error: couldn't always_inline call: argument with variably modified type
	c(0); // CHECK: !/error/
	d(0); // CHECK: error: couldn't always_inline call: argument with variably modified type
}
