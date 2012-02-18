#ifndef __UNISTD_H
#define __UNISTD_H

#include "macros.h"

#ifdef __TYPEDEFS_WORKING
typedef int pid_t;
#else
#define pid_t int
#endif

int read( int fd, void *p, int size);
int write(int fd, void *p, int size);
int close(int fd);

int   brk(void *);
void *sbrk(int inc);

pid_t fork(void);
pid_t getpid(void);

int unlink(const char *);
int rmdir( const char *);

int pipe(int [2]);

extern char **environ;

#endif
