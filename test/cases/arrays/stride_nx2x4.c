// RUN: %ocheck 0 %s

int x[2][2][4];

main()
{
#include "../ocheck-init.c"
	return x[1][1][3]; // only one dereference
}
