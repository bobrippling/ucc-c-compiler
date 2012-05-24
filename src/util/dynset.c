#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#include "alloc.h"
#include "dynset.h"
#include "dynarray.h"
#include "util.h"

void dynset_add(void ***setp, void *item, int (*cmp)(const void *, const void *))
{
	void **set = *setp;
	void **i;

	UCC_ASSERT(item, "dynset_add(): adding NULL");

	if(!set){
		/* unconditionally add */
		dynarray_add(setp, item);
		return;
	}

	for(i = set; *i; i++)
		if(!cmp(*i, item))
			return; /* already in */

	dynarray_add(setp, item);
}
