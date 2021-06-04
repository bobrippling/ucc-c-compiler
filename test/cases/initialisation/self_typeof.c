// RUN: %ocheck 3 %s

main()
{
#include "../ocheck-init.c"
	int i = (__typeof(i))3;

	return i;
}
