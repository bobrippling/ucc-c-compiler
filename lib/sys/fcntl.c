#include "fcntl.h"
#include "../stdarg.h"
#include "../syscalls.h"

int open(const char *fname, int mode, ...)
{
	int flag = 0;
	if(mode & O_CREAT){
		va_list l;
		va_start(l, mode);
		flag = va_arg(l, int);
		va_end(l);
	}
	return __syscall(SYS_open, fname, mode, flag);
}
