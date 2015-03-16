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

	if(in_array(arg, wcpp))
		owner |= W_OWNER_CPP;

	if(in_array(arg, wcc1))
		owner |= W_OWNER_CC1;

	return owner;
}
