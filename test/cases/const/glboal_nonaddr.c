// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

int a, b;

f()
{
	a = a || b; // ensure this isn't confused for &a || &b
}

g()
{
	a = &a || &b;
}

main()
{
#include "../ocheck-init.c"
	f();
	if(a || b)
		abort();

	g();
	if(!a || b)
		abort();

	return 0;
}
