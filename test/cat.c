#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/errno.h"

#include "lib/fcntl.h"
#include "lib/unistd.h"


void cat(int fd)
{
	char buf[8];
	int n;

	while((n = read(fd, buf, 8)) > 0)
		write(1, buf, n);
}

int main(int argc, char **argv)
{
	if(argc == 1){
		fprintf(stderr, "%s: reading from stdin...\n", *argv);
		cat(0);
	}else{
		int i;

		for(i = 1; i < argc; i++){
			int fd = open(argv[i], O_RDONLY);
			if(fd == -1){
				fprintf(stderr, "open \"%s\": %s\n", argv[i], strerror(errno));
			}else{
				cat(fd);
				close(fd);
			}
		}
	}

	return 0;
}
