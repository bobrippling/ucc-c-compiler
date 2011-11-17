#include "os.h"

#ifdef __FreeBSD__
#define SYS_exit   1
#define SYS_read   3
#define SYS_write  4
#define SYS_open   5
#define SYS_close  6

#else
#ifdef __MACOSX__
#define SYS_exit  0x2000001
#define SYS_read  0x2000003
#define SYS_write 0x2000004
#define SYS_open  0x2000005
#define SYS_close 0x2000006

#else
#define SYS_exit  60
#define SYS_read   0
#define SYS_write  1
#define SYS_open   2
#define SYS_close  3

#endif
#endif

extern int __syscall(int, ...);
