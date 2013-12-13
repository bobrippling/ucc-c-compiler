#include <stdio.h>
#include <fcntl.h>

main()
{
	FILE *f;
	int fd = open("goto_star.c", 0);
	char buf[256];

	if(fd == -1)
		return 1;

	f = fdopen(fd, "r");

	fread(buf, 256, 1, f);

	printf("%s\n", buf);

	fclose(f);

	return 0;
}
