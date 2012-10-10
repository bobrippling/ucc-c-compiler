#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main()
{
	int *p, *q;

	q = p = malloc(2 * sizeof *p);

	if(!p){
		perror("malloc()");
		abort();
	}

	*p = 4;
	q++;
	*q = 5;

	assert(*q + *p == 9);
	return 0;
}
