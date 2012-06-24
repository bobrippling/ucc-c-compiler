#include "string.h"
#include "limits.h"
#include "stdlib.h" // MIN

static const char *_errs[] = {
#include "string_strerrs.h"
};


size_t strlen(const char *s)
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

int strncmp(const char *a, const char *b, size_t n)
{
	while(n > 0){
		int diff;
		char ac, bc;

		ac = *a;
		bc = *b;

		if(ac + bc == 0) /* both '\0' */
			return 0;
		else if(!ac)
			return -1;
		else if(!bc)
			return 1;

		diff = ac - bc;
		if(diff)
			return diff;

		n--;
		a++;
		b++;
	}
	return 0;
}

int strcmp(const char *a, const char *b)
{
	return strncmp(a, b, UINT_MAX);
}

char *strchr(const char *s, char c)
{
	while(*s)
		if(*s == c)
			return s;
		else
			s++;
	return NULL;
}

void *memset(void *p, unsigned char c, size_t len)
{
	void *const start = p;
	// TODO: asm / duff's device
	while(len > 0)
		*(char *)p++ = c;
	return start;
}

void *memcpy(char *to, const char *from, size_t count)
{
#ifdef DUFF
	char *const ret = to;
	size_t n = (count + 7) / 8;

	switch(count % 8){
		case 0: do{ *to++ = *from++;
		case 7:     *to++ = *from++;
		case 6:     *to++ = *from++;
		case 5:     *to++ = *from++;
		case 4:     *to++ = *from++;
		case 3:     *to++ = *from++;
		case 2:     *to++ = *from++;
		case 1:     *to++ = *from++;
		}while(--n>0);
	}
	return ret;
#else
	asm("TODO");//TODO
#endif
}

char *strcpy(char *dest, const char *src)
{
	memcpy(dest, src, strlen(src));
}

char *strncpy(char *dest, const char *src, size_t n)
{
	const size_t len = strlen(src);
	memcpy(dest, src, MIN(n, len));
}
