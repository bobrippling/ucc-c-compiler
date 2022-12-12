// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

x = 56e5;

float y = 1.2e3;

z = 7959571e-3;

float q = 1e3f;

main()
{
#include "../ocheck-init.c"
	if(x != 5600000)
		abort();
	if(y != 1200)
		abort();
	if(z != 7959)
		abort();
	return 0;
}
