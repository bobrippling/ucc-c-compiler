#include <stdio.h>
#include <string.h>
#include <stdlib.h>

main()
{
	// __progname seems to segfault - pointer value isn't read correctly
	char *p = strchr("hello", 'l');
	printf("hello %s from %s\n", p, __progname);
	return 3;
}
