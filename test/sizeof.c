#include <assert.h>
main()
{
	char array[5];
	int i;
	void *p;
	int  *pi;
	char c;

	assert(sizeof array - 1 == 4);

	/*sizeof int;*/
	assert(sizeof(int) == 8);

	assert(sizeof i    == 8);
	assert(sizeof(i)   == 8);

	assert(sizeof p    == 8);
	assert(sizeof(p)   == 8);

	assert(sizeof pi   == 8);
	assert(sizeof(pi)  == 8);

	assert(sizeof c    == 1);
	assert(sizeof(c)   == 1);
}
