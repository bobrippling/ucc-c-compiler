#include "dirent.h"
#include "sys/fcntl.h"
#include "unistd.h"
#include "stdlib.h"
#include "errno.h"

#include "ucc_attr.h"

struct __DIR
{
	int fd;
};

int dirfd(DIR *dirp)
{
	return dirp->fd;
}

DIR *opendir(const char *name)
{
	int fd = open(name, O_DIRECTORY | O_RDONLY);
	if(fd == -1)
		return NULL;
	return fdopendir(fd);
}

DIR *fdopendir(int fd)
{
	DIR *d = malloc(sizeof *d);
	if(!d){
		const int e = errno;
		close(fd);
		errno = e;
		return NULL;
	}
	d->fd = fd;
	return d;
}

int closedir(DIR *d)
{
	const int r = close(d->fd);
	free(d);
	return r;
}

struct dirent *readdir(DIR *dirp __unused)
{
	// TODO
	return NULL;
}

void rewinddir(DIR *dirp __unused)
{
	// TODO
}

void seekdir(DIR *dirp __unused, long pos __unused)
{
	// TODO
}

long telldir(DIR *dirp __unused)
{
	// TODO
	return -1;
}

int scandir(const char *restrict dir __unused,
						struct dirent ***restrict namelist __unused,
						int (*selector)(const struct dirent *) __unused,
						int (*cmp)(const struct dirent **, const struct dirent **) __unused)
{
	// TODO
	return -1;
}
