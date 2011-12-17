#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

main()
{
	int pid = fork();

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
			waitpid(-1, &stat, 0);
			ec = WEXITSTATUS(stat);
			printf("yo from parent, child (pid %d) ret %d (exit status %d)\n", pid, stat, ec);
			return ec;
		}
	}
}
