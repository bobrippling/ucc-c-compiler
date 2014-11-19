// RUN: %ocheck 0 %s

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
	int c = 5;

	if(f(c).i != 7)
		abort();

	if(f(8).i == 012)
		;
	else
		abort();

	return 0;
}
