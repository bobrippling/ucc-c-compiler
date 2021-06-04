// RUN: %ocheck 6 %s
main()
{
#include "../../ocheck-init.c"
	int x[10] = { 1, [3] = 5 };
	int t = 0;
	for(int i = 0; i < 10; i++)
		t += x[i];
	return t;
}
