#include "os.h"

#ifdef __FreeBSD__
#  include "syscalls_32.h"
#else
#  ifdef __MACOSX__
#    include "syscalls_mac.h"
#  else
/* TODO: switch between 32 and 64 for linux */
#    include "syscalls_64.h"
#  endif
#endif

extern int __syscall(int, ...);
