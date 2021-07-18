// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));
main()
{
#include "../ocheck-init.c"
	int i = -5 % 2;

	if(i != -1)
		abort();

	return 0;
}
