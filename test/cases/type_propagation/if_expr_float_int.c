// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f(int b)
{
	return b ? 5 : 2.3;
}

main()
{
#include "../ocheck-init.c"
	if(f(0) != 2)
		abort();
	if(f(1) != 5)
		abort();

	return 0;
}
