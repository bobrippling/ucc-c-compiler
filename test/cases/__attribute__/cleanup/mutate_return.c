// RUN: %ocheck 5 %s

void clean(int *p)
{
	*p = 2;
}

main()
{
#include "../../ocheck-init.c"
	int x __attribute((cleanup(clean))) = 5;
	return x;
}
