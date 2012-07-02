#include <stdio.h>
#include <stdlib.h>

int main()
{
	int i;

	for(i = 0; i < 20; i++){
		int j;
		void *p;

		p = malloc(512);

		printf("[%02d] malloc(512) = %p\n", i, p); // 8 per page

		for(j = 0; j < 512; j++)
			((char *)p)[j] = 0; // make sure we can access it
	}

	return 0;
}
