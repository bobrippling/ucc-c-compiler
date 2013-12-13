#include "string.h"
#include "limits.h"
#include "stdlib.h" // MIN

static const char *_errs[] = {
#include "string_strerrs.h"
};


size_t strnlen(const char *s, size_t lim)
{
	size_t i = 0;
	while(*s++)
		if(++i == lim)
			return lim;
	return i;
}

size_t strlen(const char *s)
{
	return strnlen(s, -1UL);
}

const char *strerror(int eno)
{
	// TODO: bounds check + snprintf
	return _errs[eno - 1];
}

int memcmp(const void *va, const void *vb, size_t n)
{
	const unsigned char *a = va, *b = vb;
	while(n > 0){
		int diff;
		unsigned char ac, bc;

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

int strncmp(const char *a, const char *b, size_t n)
{
	//size_t max = MAX(strnlen(a, n), strnlen(b, n)); - waiting for cpp merge
	size_t max_a = strnlen(a, n);
	size_t max_b = strnlen(b, n);

	return memcmp(a, b, MAX(max_a, max_b));
}

int strcmp(const char *a, const char *b)
{
	return strncmp(a, b, UINT_MAX);
}

char *strchr(const char *s, char c)
{
	while(*s)
		if(*s == c)
			return (char *)s; /* the arg becomes non-const */
		else
			s++;
	return NULL;
}

void *memset(void *p, unsigned char c, size_t len)
{
	char *s = p;
	// TODO: asm / duff's device
	while(len-- > 0)
		*(unsigned char *)s++ = c;
	return p;
}

void *memcpy(void *v_to, const void *v_from, size_t count)
{
#define DUFF
#ifdef DUFF
	char *to = v_to, *from = v_from;
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
	/* TODO: repnz movsb */
	//asm("lib/string/todo");//TODO
#endif
}

char *strcpy(char *dest, const char *src)
{
	memcpy(dest, src, strlen(src));
	return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	const size_t len = strlen(src);
	memcpy(dest, src, MIN(n, len));
	return dest;
}
