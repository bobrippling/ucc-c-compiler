// RUN: %ocheck 3 %s

f(int x[])
{
	return x[2];
}

main()
{
#include "../../ocheck-init.c"
	int x[] = { 1, 2, 3 };
	return f(x);
}
