// RUN: %ocheck 3 %s
main()
{
#include "../ocheck-init.c"
	struct A { int i, j; };

	struct A x, y = { 1, 2 };

	x = y;

	return x.i + x.j;
}
