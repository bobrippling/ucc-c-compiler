#include "os.h"

#ifdef __FreeBSD__
#define SYS_exit   1
#define SYS_read   3
#define SYS_write  4

#else

#ifdef __MACOSX__
#define SYS_exit  0x2000001
#define SYS_read  0x2000003
#define SYS_write 0x2000004

#else
#define SYS_exit  60
#define SYS_read  0
#define SYS_write 1

#endif
#endif

extern int __syscall(int, ...);
