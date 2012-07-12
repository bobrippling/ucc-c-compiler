#include <stdlib.h>

main(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++)
		printf("%d\n", atoi(argv[i]));

	printf("%d\n", atoi("52"));

	return 0;
}
