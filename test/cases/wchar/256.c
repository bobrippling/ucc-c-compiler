// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
#include "../ocheck-init.c"
	volatile int x = L'\x0000000100';

	if(x != 0x100)
		abort();

	return 0;
}
