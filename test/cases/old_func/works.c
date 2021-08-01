// RUN: %ocheck 5 %s
f(i);

f(i)
	int i;
{
	return i;
}

g(i, j)
	int *j;
{
	return *j + i;
}

main()
{
#include "../ocheck-init.c"
	return f(5);
}
