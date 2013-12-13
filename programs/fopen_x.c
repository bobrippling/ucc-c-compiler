#include <stdio.h>
#include <errno.h>

main()
{
	FILE *f = fopen("/tmp/tim", "xb");

	if(!f){
		fprintf(stderr, "couldn't open: %d\n", errno);
		return 1;
	}

	printf("success\n");
	fclose(f);
	return 0;
}
