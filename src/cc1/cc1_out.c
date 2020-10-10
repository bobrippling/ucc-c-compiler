#include <stdio.h>

#include "../util/dynmap.h"
#include "../util/alloc.h"

#include "cc1_out.h"

int cc1_outsections_add(const struct section *sec)
{
	extern dynmap *cc1_outsections; /* struct section* => (int*)1 [truthy] */
	struct section *alloc;

	if(dynmap_get(const struct section *, int *, cc1_outsections, sec))
		return 0;

	alloc = umalloc(sizeof *alloc);
	memcpy_safe(alloc, sec);
	(void)dynmap_set(const struct section *, int *, cc1_outsections, (const struct section *)alloc, (int *)1);

	return 1;
}
