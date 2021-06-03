// RUN: %ocheck 5 %s

int i;

int *x()
{
	return &i;
}

main()
{
#include <ocheck-init.c>
	*x() = 5;

	return i;
}
