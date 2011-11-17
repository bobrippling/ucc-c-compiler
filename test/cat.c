#include "lib/fcntl.h"
#include "unistd.h"
#include "stdio.h"

cat(int fd)
{
	char buf[8];
	int n;

	while(n = read(fd, buf, 8) > 0)
		write(1, buf, n);
}

main(int argc, char **argv)
{
	int fd, i;

	if(argc == 1){
		printf("%s: reading from stdin...\n", *argv);
		cat(0);
	}else{
		for(i = 1; i < argc; i++){
			fd = open(argv[i], O_RDONLY, 0644);
			if(fd == -1){
				fprintf(stderr, "open \"%s\": [err]\n", argv[i]);
			}else{
				cat(fd);
				close(fd);
			}
		}
	}

	return 0;
}
