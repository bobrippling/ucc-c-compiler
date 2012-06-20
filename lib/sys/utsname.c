#include "../syscalls.h"
#include "utsname.h"

#ifdef SYS_uname
int uname(struct utsname *a)
{
	return __syscall(SYS_uname, a);
}
#else
# warning uname syscall unavailable on this platform
#endif
