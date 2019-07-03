#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

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

void rtrim(char *s)
{
	size_t l = strlen(s);

	while(l > 0 && isspace(s[l-1]))
		l--;

	s[l] = '\0';
}
