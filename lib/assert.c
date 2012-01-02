#include "stdio.h"
#include "assert.h"

void __assert_fail(int line, const char *fname, const char *func)
{
	fprintf(stderr, "assertion failure at %s:%d (%s)\n", line, fname, func);
	abort();
}
