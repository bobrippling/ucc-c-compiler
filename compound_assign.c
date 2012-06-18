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
	i = 0;
	j = 10;

	*f() += 2;

	assert(i == 2 && j == 10);
}
