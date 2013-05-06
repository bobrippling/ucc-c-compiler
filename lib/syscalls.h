#ifndef SYSCALLS_H
#define SYSCALLS_H

/* TODO: cygwin */

#ifdef __FreeBSD__
#  include "syscalls_32.h"
#else
#ifdef __DARWIN__
#  include "syscalls_darwin.h"
#else
#ifdef __x86_64__
#  include "syscalls_64.h"
#else
#  include "syscalls_32.h"
#endif
#endif
#endif

extern long __syscall(int, ...);

#endif
