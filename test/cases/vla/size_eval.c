// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

calls;

g()
{
	if(calls)
		abort();
	calls = 1;
	return 1;
}

f()
{
	int x[g()];
	return sizeof x;
}

main()
{
#include "../ocheck-init.c"
	f();
	if(calls != 1)
		abort();
	return 0;
}
