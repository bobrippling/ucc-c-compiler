// RUN: %ocheck 2 %s

main()
{
#include "../../../ocheck-init.c"
	int x[] = {
		[1 ... 2] = 1
	};

	return x[0] + x[1] + x[2];
}
