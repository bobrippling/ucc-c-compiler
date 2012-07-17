/* ucc: -fno-const-fold */
#include <stdio.h>
#include <assert.h>

int f()
{
	return 3;
}

main()
{
	int i;
	i = f() > 2;

	assert(i == 1);
	assert((5 && 2) == 1);

	return 0;
}
