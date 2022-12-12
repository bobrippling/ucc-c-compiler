// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

main()
{
#include "../../ocheck-init.c"
	static int x = sizeof(int *){ (int[]){ 1,2,3 } };
	if(x != sizeof(int *))
		abort();
	return 0;
}
