#include <sys/fcntl.h>
#include <syscalls.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct linux_dirent
{
#ifdef GOT_LONG
	unsigned long  d_ino;     /* Inode number */
	unsigned long  d_off;     /* Offset to next linux_dirent */
	unsigned short d_reclen;  /* Length of this linux_dirent */
#else
	unsigned int   d_ino[2];
	unsigned int   d_off[2];
	unsigned char  d_reclen[2];
#endif

	char           d_name[];  /* Filename (null-terminated) */
	/* length is actually (d_reclen - 2 - offsetof(struct linux_dirent, d_name) */
	/*
	char           pad;       // Zero padding byte
	char           d_type;    // File type (only since Linux 2.6.4; offset is (d_reclen - 1))
	*/

};

#define getdents(fd, s, n) __syscall(SYS_getdents, fd, s, n)
#define DIE(t, s) if(t){printf(s ": %s\n", strerror(errno)); return 1;}

main()
{
	void *ents;
	int n, b;
	int fd;

	ents = malloc(n = 32768);
	malloc(32768);

	fd = open(".", O_DIRECTORY);

	DIE(fd == -1, "open");

	n = getdents(fd, ents, n);

	close(fd);

	DIE(n == -1, "getdents");

  for(b = 0; b < n; ){
		struct linux_dirent *ent;

		ent = ents + b;

		b += ent->d_reclen[0];

		//printf("reclen = { %d, %d }\n", ent->d_reclen[0], ent->d_reclen[1]);
		printf("b=%d (+ %d [%d])\n", b,
				ent->d_reclen[0],
				ent->d_reclen[1]);
	}
}
