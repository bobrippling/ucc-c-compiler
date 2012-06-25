#include <stdarg.h>
#include "lib/syscalls.h"

execve(const char *path, char *const argv[], char *const envp[])
{
}

execl(const char *path, const char *argv0, ...)
{
	va_list l;
	int n, r;
	const char **argv;

	for(va_start(l, argv0), n = 1; va_arg(l, const char *); n++);
	va_end(l);

	argv = malloc(n * sizeof *argv); // TODO: error

	argv[0] = argv0;

	for(va_start(l, argv0), n = 1; argv[n] = va_arg(l, const char *); n++);
	va_end(l);

	r = execve(path, argv, envp);
	free(argv);

	return r;
}

main()
{
	int p = fork();

	if(!p){
		printf("child, pid = %d\n", getpid());
		execl("/bin/sh", "sh", "-c", "echo hi $$", 0);
		perror("execv");
		_Exit(5);
	}

	printf("pid = %d\n", getpid());
	waitpid(0, 0, 0);
}
