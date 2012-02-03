#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

void __assert_fail(const char *src, int line, const char *fname, const char *func)
{
	// a.out: assert.c:5: main: Assertion `argc == 2' failed.
	fprintf(stderr, "%s: %s:%d: %s: Assertion '%s' failed.\n",
			__progname, fname, line, func, src);
	abort();
}
