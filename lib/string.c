#include "string.h"

static const char *_errs[] = {
#include "string_strerrs.h"
};


int strlen(char *s)
{
	int i = 0;
	while(*s++)
		i++;
	return i;
}

const char *strerror(int eno)
{
	//extern const char *_errs[];
	return _errs[eno - 1];
}
