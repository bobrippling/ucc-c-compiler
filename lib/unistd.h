#ifndef __UNISTD_H
#define __UNISTD_H

#include "macros.h"
#include "sys/types.h"

typedef int pid_t;

int read( int fd, void *p, int size);
int write(int fd, void *p, int size);
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

int pipe(int [2]);

extern char **environ;

#endif
