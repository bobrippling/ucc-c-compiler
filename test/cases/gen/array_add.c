// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

char g[1];
const char *f()
{
	return g + 7;
}

main()
{
#include "../ocheck-init.c"
	char *p = g;
	if(f() != p + 7)
		abort();
	return 0;
}
