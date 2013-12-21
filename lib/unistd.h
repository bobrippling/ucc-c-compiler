#ifndef __UNISTD_H
#define __UNISTD_H

#include "macros.h"
#include "sys/types.h"
#include "sys/time.h"
#include "ucc_attr.h"

typedef int pid_t;

int read( int fd, void *p, int size);
int write(int fd, const void *p, int size);
int close(int fd);

#ifndef __DARWIN__
int   brk(void *);
void *sbrk(int inc);
#endif

pid_t fork(void);
pid_t getpid(void);

int unlink(const char *);
int rmdir( const char *);

ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize);
int link(const char *from, const char *new);
int symlink(const char *link, const char *new);

int pipe(int [static 2]);

extern char **environ;

#ifndef SEEK_SET
# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2
#endif

off_t lseek(int fd, off_t offset, int whence);

// FIXME: nanosleep, usleep
unsigned int sleep(unsigned int);
int usleep(useconds_t usec);

void _exit(int) __dead;

#endif
