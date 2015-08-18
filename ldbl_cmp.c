int f(long double a, long double b)
#ifdef IMPL
{

	return a >= b;
}
#else
;
#endif

#ifndef IMPL
int printf(const char *, ...);

void try(int ai, int bi, int expected)
{
	long double a, b;

	a = ai;
	b = bi;

	printf("a=%Lf\n", a);
	printf("b=%Lf\n", b);

	int t = f(a, b);
	if(t != expected)
		printf("\e[35m");
	printf("a >= b: %d (expected %d)\n", t, expected);
	if(t != expected)
		printf("\e[m");
}

#define true 1
#define false 0
int main()
{
	try(1, 2, false);
	try(1, 1, true);
	try(1, 0, true);
}
#endif
