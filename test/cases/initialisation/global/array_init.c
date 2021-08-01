// RUN: %ocheck 1 %s

int x[4] = { 1 };

main()
{
#include "../../ocheck-init.c"
	return x[0] + x[1] + x[2] + x[3];
}
