// RUN: %ocheck 0 %s -fno-semantic-interposition
void abort(void) __attribute__((noreturn));

typedef struct A { long i, j, k; } A;

__attribute((always_inline))
inline A f(int cond)
{
	A local = { .i = 5 };
	if(cond == 8)
		return (A){ .i = 012 };
	return cond ? (A){ .i = 7 } : local;
}

main()
{
#include "../ocheck-init.c"
	int c = 5;

	if(f(c).i != 7)
		abort();

	if(f(8).i == 012)
		;
	else
		abort();

	return 0;
}
