#include <assert.h>

//#define SZ_INT sizeof(int)
//#define SZ_VP  sizeof(void *)
#define SZ_INT 8
#define SZ_VP  8

main()
{
	int x[4];
	int *y[4];
	int (*z)[4];

	assert(sizeof(x) == 4 * SZ_INT);
	assert(sizeof(y) == 4 * SZ_VP);
	assert(sizeof(z) == SZ_VP);
}
