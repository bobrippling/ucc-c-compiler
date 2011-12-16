#include <stdlib.h>

void *malloc2(unsigned s)
{
	void *p = malloc(s);
	//printf("malloc(%d) = %p\n", s, p);
	return p;
}


main()
{
	int *p = malloc2(sizeof p);
	*p = 5;
	//printf("p = %p, *p = %d\n", p, *p);
	return *p;
}
