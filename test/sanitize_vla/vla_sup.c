#include "vla_sup.h"

g()
{
	return 2;
}

void fill(void *v, size_t sz, int with)
{
	char *p = v;
	while(sz --> 0)
		*p++ = with;
}
