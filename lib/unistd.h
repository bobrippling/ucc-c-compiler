int read( int fd, void *p, int size);
int write(int fd, void *p, int size);
int close(int fd);

int   brk(void *);
void *sbrk(int inc);

#define NULL (void *)0
