#define SZ_INT sizeof(int)
#define SZ_VP  sizeof(void *)

extern void abort(void);
#define SZ_ASSERT(var, sz) if(sizeof(var) != (sz)) abort()

main()
{
	int x[4];
	int *y[4];
	int (*z)[4];
	int *****p;

	SZ_ASSERT(x, 4 * SZ_INT);
	SZ_ASSERT(y, 4 * SZ_VP);
	SZ_ASSERT(z, SZ_VP);
	SZ_ASSERT(p, SZ_VP);
}
