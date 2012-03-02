#include <assert.h>

#define SUM(a, b) ({           \
			int i, r = 0;            \
			for(i = 1; i <= 5; i++)  \
				r += i;                \
			r;                       \
		})


main()
{
	int i;

	i = SUM(1, 5);

	assert(i == 15);

	({(void)printf("hello\n");});

	return i - 15;
}
