#include <stdlib.h>
#include <assert.h>

main()
{
	int i;
#ifdef ARRAY
	void *pointer[2];

	pointer = malloc(2 * sizeof *pointer);
	pointer[1] = 5;

	i = ++*(int *)pointer[1];
#else
	void *p;

	//*(int *)(p = malloc(sizeof(int))) = 5;
	p = malloc(sizeof(int));
	if(!p){
		perror("malloc()");
		return 1;
	}
	*(int *)p = 5;

	i = ++*(int *)p; // Bug - this line (?)
#endif

	assert(i == 6);
	assert(*(int *)p == 6);
}
