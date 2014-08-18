#include "dynvec.h"
#include "alloc.h"

void *dynvec_sz_add(void *p, size_t *sz, size_t sz1)
{
	void **pp = p;

	*sz += sz1;
	*pp = urealloc1(*pp, *sz);

	return (char *)*pp + *sz - sz1;
}
