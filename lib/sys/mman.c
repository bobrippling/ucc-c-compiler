#include "types.h"
#include "mman.h"
#include "../syscalls.h"

void *mmap(void *addr, size_t len,
		int prot, int flags,
		int fd, off_t offset)
{
	return (void *)__syscall(SYS_mmap, addr, len, prot, flags, fd, offset);
}

int munmap(void *addr, size_t len)
{
	return __syscall(SYS_munmap, addr, len);
}
