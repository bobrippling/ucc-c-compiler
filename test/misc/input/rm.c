#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

void die(char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
	exit(1);
}

int main(int argc, char **argv)
{
	int i;

	if(argc < 2){
		die("Usage: %s files...\n", *argv);
		return 5;
	}

	for(i = 1; i < argc; i++)
		if(remove(argv[i]))
			die("remove \"%s\": %s\n", argv[i], strerror(errno));

	return 0;
}
