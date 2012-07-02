#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

main()
{
	pid_t pid = fork();

	switch(pid){
		case -1:
			fprintf(stderr, "fork(): %s\n", strerror(errno));
			return 1;

		case 0:
			printf("yo from child\n");
			exit(5);

		default:
		{
			int stat, ec;
			pid_t ret;

			ret = waitpid(-1, &stat, 0);

			printf("yo from parent, fork() = %d, waitpid(%d) = %d (exit status %d)\n",
					pid, stat, ret, ec = WEXITSTATUS(stat));

			return ec;
		}
	}
}
