#include <assert.h>

typedef struct
{
	int i;
	char c;
	int j;
} A;

main()
{
	A a;
	void *p;

	a.i = 123456;
	a.c = 2;
	a.j = 654321;

	// assuming standard struct layout...
	p = &a;

	assert(*(int *)p == 123456);
	p += sizeof(int);
	assert(*(char *)p == 2);
	p += sizeof(int); /* ? */
	assert(*(int *)p == 654321);
}
