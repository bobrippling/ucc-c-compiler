#include <assert.h>

#define SUM(a, b) ({           \
			int i, r = 0;            \
			for(i = a; i <= b; i++)  \
				r += i;                \
			r;                       \
		})


main()
{
	int i;

	i = SUM(1, 5);

	assert(i == 15);

	({(void)printf("hello\n");});

	({
	 int i;
	 i = 2;
	});

	assert(i - 15 == 0);
}
