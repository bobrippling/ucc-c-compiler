// RUN: %ucc -fno-const-fold -o %t %s
// RUN: %t

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

	int a = 5, b = 2;
	assert((a && b) == 1);

	return 0;
}
