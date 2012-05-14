#include <stdio.h>
#include <string.h>

#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "namespace.h"

char *namespace_to_str(char **ns_path)
{
	static char *buf;
	char *pos, **i;
	int len;

	for(len = 1, i = ns_path; *i; i++)
		len += strlen(*i) + i[1] ? 2 : 0;

	pos = buf = urealloc(buf, len);

	for(i = ns_path; *i; i++)
		pos += sprintf(pos, "%s%s",
				*i, i[1] ? "::" : "");

	return buf;
}

char **namespace_none(char *sp)
{
	char **r = NULL;
	dynarray_add((void ***)&r, sp);
	return r;
}
