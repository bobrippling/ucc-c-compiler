#include "string.h"
#include "limits.h"

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
	// TODO: bounds check + snprintf
	return _errs[eno - 1];
}

int strncmp(char *a, char *b, size_t n)
{
	while(n > 0){
		int x = *b - *a;
		if(x)
			return x;
		n--;
		a++;
		b++;
	}
	return 0;
}

int strcmp(char *a, char *b)
{
	return strncmp(a, b, UINT_MAX);
}

char *strchr(char *s, char c)
{
	while(*s)
		if(*s == c)
			return s;
		else
			s++;
	return NULL;
}
