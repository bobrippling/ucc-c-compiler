// RUN: %ocheck 6 %s

main()
{
#include "../../ocheck-init.c"
	struct A
	{
		int i, j, k;
	} a = { 1, 2, 3 }, b = a;

	return b.i + b.j + b.k;
}
