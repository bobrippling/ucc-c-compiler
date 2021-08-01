// RUN: %ocheck 16 %s

main()
{
#include "../../ocheck-init.c"
	struct
	{
		int x : 4, y : 4;
	} a = { .y = 1 };

	return *(char *)&a;
}
