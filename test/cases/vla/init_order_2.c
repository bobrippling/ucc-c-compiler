// RUN: %ocheck 6 %s

main()
{
#include "../ocheck-init.c"
	int i = 5;
	int x[i][++i];

	return i;
}
