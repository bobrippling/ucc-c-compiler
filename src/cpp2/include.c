#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/util.h"

#include "include.h"

/* CPP_DIE() */
#include "main.h"
#include "preproc.h"

static char **include_dirs;

void include_add_dir(char *d)
{
	dynarray_add(&include_dirs, d);
}

FILE *include_fopen(const char *fnam)
{
	FILE *f;
	char *rslash = strrchr(fnam, '/');

	if(!(rslash ? rslash[1] : *fnam))
		CPP_DIE("empty filename");

	f = fopen(fnam, "r");
	if(f)
		return f;

	if(errno != ENOENT)
		CPP_DIE("open: %s:", fnam);

	return NULL;
}

FILE *include_search_fopen(const char *cd, const char *fnam, char **ppath)
{
	FILE *f = NULL;
	int i;

	for(i = 0; include_dirs && include_dirs[i]; i++){
		char *path = ustrprintf(
				"%s/%s/%s",
				*include_dirs[i] == '/' ? "" : cd,
				include_dirs[i],
				fnam);

		f = include_fopen(path);
		if(f){
			*ppath = path;
			break;
		}
		free(path);
	}

	return f;
}
