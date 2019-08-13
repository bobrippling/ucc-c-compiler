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
#define ALIAS X
#define GROUP X
#include "../cc1/warnings.def"
#undef X
#undef ALIAS
#undef GROUP
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
	int is_werror = 0; /* cpp doesn't currently handle -Werror or -Werror=..., so filter these out */

	/* handle no- */
	if(!strncmp(arg, "no-", 3))
		arg += 3;

	/* handle error=... */
	if(!strncmp(arg, "error=", 6)){
		is_werror = 1;
		arg += 6;
	}

	/* handle all, extra and everything and error */
	if(!strcmp(arg, "all")
	|| !strcmp(arg, "everything"))
	{
		return W_OWNER_CC1 | (is_werror ? 0 : W_OWNER_CPP);
	}

	if(!strcmp(arg, "extra")
	|| !strcmp(arg, "error"))
	{
		return W_OWNER_CC1;
	}

	/* handle gnu */
	if(!strcmp(arg, "gnu"))
		return W_OWNER_CC1;

	if(!is_werror && in_array(arg, wcpp))
		owner |= W_OWNER_CPP;

	if(in_array(arg, wcc1))
		owner |= W_OWNER_CC1;

	return owner;
}
