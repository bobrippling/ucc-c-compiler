// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

g(int a)
{
	return a + 1;
}

f(int a, int b, int c)
{
	if(a != 2 || b != 2 || c != 3)
		abort();
}

int fseek(int a, int b, int c)
{
	f(g(a), b, c);
}

main()
{
#include "../ocheck-init.c"
	fseek(1, 2, 3);
	return 0;
}
