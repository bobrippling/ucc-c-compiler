#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "../util/dynmap.h"
#include "../util/alloc.h"
#include "../util/path.h"

#include "deps.h"

static dynmap *depset;

void deps_add(const char *d)
{
	if(dynmap_exists(char *, depset, (char *)d))
		return;

	if(!depset)
		depset = dynmap_new((dynmap_cmp_f *)strcmp);

	dynmap_set(
			char *, void *,
			depset,
			/* not lost: */ustrdup(d), NULL);
}

void deps_dump(const char *file)
{
	/* replace ext if present, otherwise tac ".o" on */
	const char *basename = strrchr(file, '/');
	const char *ext;
	char *obj;
	size_t i;

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
	free(obj);

	for(i = 0; (obj = dynmap_key(char *, depset, i)); i++)
		printf(" %s", obj);
	putchar('\n');
}
