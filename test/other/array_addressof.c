#include <assert.h>

set(int *p, int v)
{
	*p = v;
}

main()
{
	int x[2];

	x[1] = 5;

	set( x   , 2);
	set(&x[1], 3);

	assert(0 == 1 + (x[0] - x[1]));
}
