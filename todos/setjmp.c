#include <stdio.h>
#include <setjmp.h>

static jmp_buf excp;

void f()
{
	char buf[8];

	if(fgets(buf, sizeof buf, stdin))
		printf("read %s\n", buf);
	else
		longjmp(excp, 1);
}

main()
{
	int r;

	switch(setjmp(excp)){
		case 0:
			f();
			r = 0;
			break;

		default:
			printf("exception!\n");
			r = 1;
	}

	return r;
}
