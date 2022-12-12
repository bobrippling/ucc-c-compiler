// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
#include "../../ocheck-init.c"
	struct { int i, j; } x[] = {
		[0 ... 5] = { 5, 6 },
		[3] = 1,
		[4] = { 3, 4 },
	};

	if(x[0].i != 5 || x[0].j != 6
	|| x[1].i != 5 || x[1].j != 6
	|| x[2].i != 5 || x[2].j != 6
	|| x[3].i != 1 || x[3].j != 6
	|| x[4].i != 3 || x[4].j != 4
	|| x[5].i != 5 || x[5].j != 6)
		abort();

	return 0;
}
