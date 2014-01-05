#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "str.h"

char **strsplit(const char *s, const char *sep)
{
	char **ret = NULL;
	char *dup = ustrdup(s);
	char *p;

	for(p = strtok(dup, sep); p; p = strtok(NULL, sep))
		dynarray_add(&ret, ustrdup(p));

	free(dup);

	return ret;
}

char **strprepend(const char *j, char **args)
{
	char **i;
	for(i = args; i && *i; i++){
		char *old = *i;
		*i = ustrprintf("%s%s", j, old);
		free(old);
	}
	return args;
}
