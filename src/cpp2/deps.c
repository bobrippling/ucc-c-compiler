#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"

#include "deps.h"

static char **deps;

void deps_add(const char *d)
{
	dynarray_add(&deps, ustrdup(d));
}

void deps_dump(const char *file)
{
	/* replace ext if present, otherwise tac ".o" on */
	const char *basename = strrchr(file, '/');
	const char *ext;
	char *obj, **i;

	if(!basename)
		basename = file;

	ext = strrchr(basename, '.');
	if(ext){
		if(ext[1]){
			/* replace ext */
			ptrdiff_t off = ext - file + 1;
			obj = ustrdup(file);
			obj[off] = 'o';
			obj[off + 1] = '\0';
		}else{
			obj = ustrprintf("%so", file);
		}
	}else{
		/* no extension */
		obj = ustrprintf("%s.o", file);
	}

	printf("%s: %s", obj, file);

	for(i = deps; i && *i; i++)
		printf(" %s", *i);
	putchar('\n');

	free(obj);
}
