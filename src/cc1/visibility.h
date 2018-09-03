#ifndef VISIBILITY_H
#define VISIBILITY_H

enum visibility
{
	VISIBILITY_DEFAULT,
	VISIBILITY_HIDDEN,
	VISIBILITY_PROTECTED
};

int visibility_parse(enum visibility *, const char *);
const char *visibility_to_str(enum visibility);

#endif
