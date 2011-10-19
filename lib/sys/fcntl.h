#define O_RDONLY	     00
#define O_WRONLY	     01
#define O_RDWR		     02
#define O_CREAT		   0100	/* not fcntl */
#define O_EXCL		   0200	/* not fcntl */
#define O_NOCTTY	   0400	/* not fcntl */
#define O_TRUNC		  01000	/* not fcntl */
#define O_APPEND	  02000

#define O_DIRECTORY	0200000	/* Must be a directory.	 */
#define O_NOFOLLOW	0400000	/* Do not follow links.	 */
#define O_CLOEXEC   02000000 /* Set close_on_exec.  */

int open(const char *fname, int flags, int mode);
int close(int fd);
