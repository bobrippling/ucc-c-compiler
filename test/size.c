#include <stdio.h>
#include <sys/types.h>
#include <assert.h>

main()
{
	printf("int=%d\n"
         "char=%d\n"
         "void=%d\n"
         "void *=%d\n"
         "char *=%d\n"
				 ,
				 sizeof(int),
				 sizeof(char),
				 sizeof(void),
				 sizeof(void *),
				 sizeof(char *));

#define SIZE(t, n) assert(sizeof(t) == n)

	//SIZE(int   , 4);
	SIZE(char  , 1);
	SIZE(void  , 1);
	SIZE(void *, __WORDSIZE / 8);
	SIZE(char *, __WORDSIZE / 8);
}
