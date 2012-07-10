#include <stdlib.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SZ 4096

do_write()
{
	int fd;

	fd = open("file", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if(fd == -1){
		perror("open file");
		return 1;
	}

	write(fd, "hi\n", 3);

	close(fd);
}

do_read()
{
	void *p;
	int i, fd;

	fd = open("file", O_RDONLY);

	if(fd == -1){
		perror("open file");
		return 1;
	}

	p = mmap(NULL, SZ, PROT_READ, MAP_PRIVATE | MAP_FILE, fd, 0);

	if(p == MAP_FAILED){
		perror("mmap()");
		return 1;
	}

	for(i = 0; i < 3; i++){
		char c = ((char *)p)[i];
		printf("file[%d] = 0x%x '%c'\n", c, c);
	}

	if(munmap(p, SZ))
		perror("munmap()");

	if(close(fd))
		perror("close()");
}

main()
{
	int i;

	i = 0;

	i += do_write();
	i += do_read();

	remove("file");

	return i;
}
