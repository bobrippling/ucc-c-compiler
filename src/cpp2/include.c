#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"

#include "include.h"

static char **include_dirs;

void include_add_dir(char *d)
{
	dynarray_add(&include_dirs, d);
}

FILE *include_fopen(const char *cd, const char *fnam)
{
	FILE *f = NULL;
	int i;

	for(i = 0; include_dirs && include_dirs[i]; i++){
		char *path = ustrprintf(
				"%s/%s/%s",
				*include_dirs[i] == '/' ? "" : cd,
				include_dirs[i],
				fnam);

		f = fopen(path, "r");
		free(path);
		if(f)
			break;
	}

	return f;
}
