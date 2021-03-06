#include <string.h>

#include "visibility.h"

int visibility_parse(enum visibility *const e, const char *s, int allow_protected)
{
	if(!strcmp(s, "default"))
		*e = VISIBILITY_DEFAULT;
	else if(!strcmp(s, "hidden"))
		*e = VISIBILITY_HIDDEN;
	else if(!strcmp(s, "protected") && allow_protected)
		*e = VISIBILITY_PROTECTED;
	else
		return 0;

	return 1;
}

const char *visibility_to_str(enum visibility v)
{
	switch(v){
		case VISIBILITY_DEFAULT: return "default";
		case VISIBILITY_HIDDEN: return "hidden";
		case VISIBILITY_PROTECTED: return "protected";
	}
	return NULL;
}
