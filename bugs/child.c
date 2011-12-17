#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

main()
{
	int pid = fork();

	switch(pid){
		case 0:
			printf("yo from child\n");
			exit(5);

		case 1:
		{
			int stat;
			waitpid(-1, &stat, 0);
			printf("yo from parent, child ret %d\n", stat);
			return stat;
		}

		default:
			fprintf(stderr, "fork(): %s\n", strerror(errno));
			return 1;
	}
}
