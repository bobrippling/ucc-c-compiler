// RUN: %ocheck 6 %s

int *p = &(int[]){1,2,3}[1];

main()
{
#include <ocheck-init.c>
	return p[-1] + p[0] + p[1];
}
