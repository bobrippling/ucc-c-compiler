#include <stdio.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int i;
	for(i = 0; i < argc; i++)
		puts(argv[i]);
}
