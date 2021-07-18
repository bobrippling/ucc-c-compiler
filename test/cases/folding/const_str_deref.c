// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

f()
{
	int i = '\n' ^ 0;

	return i | ~1 + 0[""];
}

main()
{
#include "../ocheck-init.c"
	if(f() != -2)
		abort();

	return 0;
}
