/* ucc: -fno-const-fold */
#include <assert.h>

#define cmp_assert(t, e) ({ int i = t; die(#t, i, i != e); })

die(const char *test, int v, int die)
{
	printf("%s = %d%s\n", test, v, die ? "!" : "");
	if(die)
		abort();
}

main()
{
	cmp_assert(5 && 3, 1);
	cmp_assert(5  > 3, 1);
	cmp_assert(2  > 3, 0);
}
