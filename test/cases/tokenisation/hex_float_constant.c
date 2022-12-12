// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

float f = 0x.1p1;
double d = 0x0.3de31P3;
double g = 0xDE.488631p0;

main()
{
#include "../ocheck-init.c"
	if(f != 0.125f)
		abort();

	return 0;
}
