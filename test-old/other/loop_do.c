#include <assert.h>

main()
{
	int i, n;

	i = 5;
	n = 0;

	do
		n++;
	while(i--);

	assert(n == 6);
}
