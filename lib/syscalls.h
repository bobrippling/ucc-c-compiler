#include "os.h"

#ifdef __FreeBSD__
#  include "syscalls_32.h"
#else
#ifdef __MACOSX__
#  include "syscalls_mac.h"
#else
#ifdef __x86_64__
#  include "syscalls_64.h"
#else
#  include "syscalls_32.h"
#endif
#endif
#endif

extern int __syscall(int, ...);
