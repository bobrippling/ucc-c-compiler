// RUN: %ocheck 3 %s

struct A
{
	int i, j;
} f()
{
	return (struct A){ 1, 2 };
}

main()
{
#include "../../ocheck-init.c"
	return f().j + f().i;
}
