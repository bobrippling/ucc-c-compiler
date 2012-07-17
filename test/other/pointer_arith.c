#include <assert.h>

main()
{
	int *ia, *ib;
	char *ca, *cb;

	int i;
	char c;


	ia = &i;
	ib = &i + 5;

	assert(ib - ia == 5);
	assert((void *)ib - (void *)ia == 5 * sizeof(int));

	ca = &c;
	cb = &c + 3;

	assert(cb - ca == 3);
}
