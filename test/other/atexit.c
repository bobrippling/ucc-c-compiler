#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BYE() printf("bye from %s\n", __func__)

void p(void)
{
	BYE();
}

void q(void)
{
	BYE();
}

reg(char *sp, void (*f)(void))
{
	if(atexit(f) == -1)
		fprintf(stderr, "atexit(%s): %d\n", sp, errno); //strerror(errno));
	else
		printf("registered %s\n", sp);
}

main()
{
#define REGISTER(f) reg(#f, f)
	REGISTER(p);
	REGISTER(q);
	return 0;
}
