#include "asm.h"

int printf(const char *, ...);

static void dumpstack(void)
{
	typedef unsigned long T;
	char *p = get_rsp();

	for(int i = 0; i < 10; i++){
		printf("%p: %#lx\n", p, *(T *)p);
		p -= sizeof(T);
	}
}

int main()
{
	dumpstack();
}
