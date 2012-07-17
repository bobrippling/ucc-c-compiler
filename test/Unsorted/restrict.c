#ifndef __UCC__
#  define restrict __restrict__
#endif

typedef int *intptr;

main()
{
	int x;
	int *restrict px;
	//intptr restrict p;
	intptr restrict p;
}
