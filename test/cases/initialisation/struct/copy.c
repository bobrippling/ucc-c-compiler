// RUN: %ocheck 3 %s
struct A
{
	int i, j;
};

main()
{
#include "../../ocheck-init.c"
	struct A a = { 1, 2 }, b = a;

	return b.i + b.j;
}
