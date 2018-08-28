#include <string.h>

#include "visibility.h"
#include "../config_as.h"

int visibility_parse(enum visibility *const e, const char *s)
{
	if(!strcmp(s, "default"))
		*e = VISIBILITY_DEFAULT;
	else if(!strcmp(s, "hidden"))
		*e = VISIBILITY_HIDDEN;
	else if(!strcmp(s, "protected"))
		*e = VISIBILITY_PROTECTED;
	else
		return 0;

	return 1;
}
