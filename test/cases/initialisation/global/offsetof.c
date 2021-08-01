// RUN: %ocheck 4 %s

int x = (long)&(((struct { int i, j; } *)0)->j);

main()
{
#include "../../ocheck-init.c"
	return x;
}
