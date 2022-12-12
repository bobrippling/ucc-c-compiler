// RUN: %ocheck 0 %s
void *memset(void *b, int c, unsigned long len);
void abort(void) __attribute__((noreturn));

f(int n)
{
	short vla[4][n];

	memset(vla, 9, sizeof vla);

	typedef short (*ptr_n)[n];

	ptr_n a, b = vla + 2;

	a = vla;

	a++;
	++b;

	(*a)[1] = 2;
	(*b)[3] = 7;

	if(vla[1][1] != 2)
		abort();

	if(vla[3][3] != 7)
		abort();
}

main()
{
#include "../ocheck-init.c"
	f(10);

	return 0;
}
