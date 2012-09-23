#include <assert.h>

int i, j;

*f()
{
	static int s;

	if(s++)
		return &j;
	return &i;
}

main()
{
#ifndef TEST
	i = 0;
	j = 10;

	*f() += 2;

	assert(i == 2 && j == 10);
#else
	int i = 5;

	assert(i++ == 5);
	assert(i == 6);
	assert(++i == 7);

	i++;

	i /= 2;

	assert(i == 4);
#endif
}
