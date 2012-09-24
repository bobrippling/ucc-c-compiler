#include <assert.h>

#define SZ_INT sizeof(int)
#define SZ_VP  sizeof(void *)
//#define SZ_INT 8
//#define SZ_VP  8

#if 0
#define WRITE(x) write(1, x "\n", strlen(x) + 1)

sz_assert(int vsz, const char *s, int expected)
{
	if(vsz != expected){
		WRITE(s);
		//printf("sizeof(%s) != %d (got %d)\n", s, expected, vsz);
	}
}
#define SZ_ASSERT(var, sz) sz_assert(sizeof(var), #var, sz)
#endif

int f(int,int);

#define SZ_ASSERT(var, sz) f(sizeof(var), sz)

main()
{
	int x[4];
	int *y[4];
	int (*z)[4];

	SZ_ASSERT(x, 4 * SZ_INT);
	SZ_ASSERT(y, 4 * SZ_VP);
	SZ_ASSERT(z, SZ_VP);
}
