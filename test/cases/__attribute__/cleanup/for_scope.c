// RUN: %ocheck 0 %s

_Noreturn void abort();

int i;

void cint(int *p)
{
	if(*p != (int[]){ 3, 3, 2, 0 }[i++])
		abort();
}

#ifdef __UCC__
typedef __attribute((cleanup(cint))) int cleanup;
#else
#define cleanup __attribute((cleanup(cint))) int
#endif


void f()
{
	cleanup a = 0;

	for(cleanup b = 0; b < 2; b++){
		cleanup c = 3;
	}
}

main()
{
#include "../../ocheck-init.c"
	f();

	if(i != 4)
		abort();

	return 0;
}
