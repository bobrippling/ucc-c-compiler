#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

void __assert_fail(int line, const char *fname, const char *func)
{
	fprintf(stderr, "assertion failure at %s:%d in %s()\n", fname, line, func);
	abort();
}
