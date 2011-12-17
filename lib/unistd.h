int read( int fd, void *p, int size);
int write(int fd, void *p, int size);
int close(int fd);

int   brk(void *);
void *sbrk(int inc);

// TODO: typedef
#define pid_t int

pid_t fork(void);
//pid_t wait(int *status); FIXME
pid_t waitpid(int pid, int *status, int options);

#define NULL (void *)0
