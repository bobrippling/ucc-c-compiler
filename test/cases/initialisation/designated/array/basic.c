// RUN: %ocheck 2 %s

int x[] = {
	[5] = 2
};

main()
{
#include "../../../ocheck-init.c"
	return
		x[0] +
		x[1] +
		x[2] +
		x[3] +
		x[4] +
		x[5];
}
