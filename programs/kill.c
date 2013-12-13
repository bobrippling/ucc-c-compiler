#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <signal.h>

void die(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
	fputc('\n', stderr);

	exit(1);
}

#ifdef OLD
main(argc, argv)
	int argc;
	char **argv;
#else
main(int argc, char **argv)
#endif
{
	pid_t pid;

	if(argc != 2)
		die("Usage: %s pid", *argv);

	if(!(pid = atoi(argv[1])))
		die("%s: Invalid pid", *argv);

	if(kill(pid, SIGTERM))
		die("kill %d: %s", pid, strerror(errno));

	return 0;
}
