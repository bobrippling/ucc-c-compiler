#include <stdlib.h>
#include <stdio.h>

main(int argc, char **argv)
{
	int i;
	int ret = 0;

	for(i = 1; i < argc; i++)
		printf("atoi(\"%s\") = %d\n", argv[i], ret = atoi(argv[i]));

	return ret;
}
