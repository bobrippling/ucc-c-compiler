#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#define O_CREAT		   0100
#define O_EXCL		   0200
#define O_NOCTTY	   0400
#define O_TRUNC		  01000
#define O_APPEND	  02000

#define O_DIRECTORY	0200000
#define O_NOFOLLOW	0400000
#define O_CLOEXEC   02000000


int open(const char *fname, int mode, ...); /* third arg iff O_CREAT */
