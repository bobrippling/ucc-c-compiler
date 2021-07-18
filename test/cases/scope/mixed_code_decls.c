// RUN: %ocheck 2 %s
f(){}

main()
{
#include "../ocheck-init.c"
	int j;

	f();
	j = 2;

	int i = j;
	return i;
}
