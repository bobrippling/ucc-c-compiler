// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
#include "../ocheck-init.c"
	int x = -1;
	int y = 4294967295;

	if(x != y)
		abort();

	return 0;
}
