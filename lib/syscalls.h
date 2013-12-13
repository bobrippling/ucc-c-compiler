#ifndef SYSCALLS_H
#define SYSCALLS_H

/* TODO: cygwin */

#if defined(__FreeBSD__)
#  include "syscalls_32.h"
#elif defined(__DARWIN__)
#  include "syscalls_darwin.h"
#elif defined(__x86_64__)
#  include "syscalls_64.h"
#else
#  include "syscalls_32.h"
#endif

extern long __syscall(int, ...);

#endif
