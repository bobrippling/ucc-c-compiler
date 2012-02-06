#include <stdlib.h>
#include <assert.h>

int main()
{
	int *p, *q;

	q = p = malloc(2 * sizeof *p);

	*p = 4;
	q++;
	*q = 5;

	assert(*q + *p == 9);
}
