#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "../util/dynmap.h"
#include "../util/alloc.h"
#include "../util/path.h"

#include "deps.h"

#define LINE_MAX 75

static dynmap *depset;

void deps_add(const char *d)
{
	if(dynmap_exists(char *, depset, (char *)d))
		return;

	if(!depset)
		depset = dynmap_new((dynmap_cmp_f *)strcmp, dynmap_strhash);

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
	size_t i, len;

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
	len = strlen(obj) + strlen(file) + 2;
	free(obj);

	for(i = 0; (obj = dynmap_key(char *, depset, i)); i++){
		size_t this_len = 1 + strlen(obj);

		len += this_len;
		if(len >= LINE_MAX){
			printf(" \\\n");
			len = this_len;
		}

		printf(" %s", obj);
	}
	putchar('\n');
}
