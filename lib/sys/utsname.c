#include "../syscalls.h"
#include "utsname.h"

int uname(struct utsname *a)
{
	return __syscall(SYS_uname, a);
}
