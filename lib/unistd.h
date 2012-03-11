#ifndef __UNISTD_H
#define __UNISTD_H

#include "macros.h"

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

int pipe(int [2]);

extern char **environ;

#endif
