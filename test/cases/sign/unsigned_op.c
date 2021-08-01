// RUN: %ocheck 0 %s

main()
{
#include "../ocheck-init.c"
	return -5 < 10U; // unsigned cmp, false
}
