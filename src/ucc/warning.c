#include <string.h>

#include "warning.h"

static const char *wcpp[] = {
#define X(name, desc, flag) name,
#include "../cpp2/warnings.def"
#undef X
	NULL
};

static const char *wcc1[] = {
#define X(name, ...) name,
#include "../cc1/warnings.def"
#undef X
	NULL
};

static int in_array(const char *needle, const char *haystack[])
{
	const char **i;

	for(i = haystack; *i; i++)
		if(!strcmp(needle, *i))
			return 1;

	return 0;
}

enum warning_owner warning_owner(const char *arg)
{
	enum warning_owner owner = 0;

	/* handle no- */
	if(!strncmp(arg, "no-", 3))
		arg += 3;

	/* handle all, extra and everything and error */
	if(!strcmp(arg, "all")
	|| !strcmp(arg, "extra")
	|| !strcmp(arg, "everything")
	|| !strcmp(arg, "error"))
	{
		return W_OWNER_CC1 | W_OWNER_CPP;
	}

	/* handle gnu */
	if(!strcmp(arg, "gnu"))
		return W_OWNER_CC1;

	/* handle error=... */
	if(!strncmp(arg, "error=", 6))
		arg += 6;

	if(in_array(arg, wcpp))
		owner |= W_OWNER_CPP;

	if(in_array(arg, wcc1))
		owner |= W_OWNER_CC1;

	return owner;
}
