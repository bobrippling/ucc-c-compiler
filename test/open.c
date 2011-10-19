#include "lib/sys/fcntl.h"

int main(int argc, char **argv)
{
	int fd;

	if(argc != 2){
		printf("Usage: %s file\n", *argv);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if(fd == -1){
		//fprintf(stderr, "open %s: %s\n", fname, "???");
		printf("Can't open %s\n", argv[1]);
		return 1;
	}

	close(fd);

	return 0;
}
