#ifndef __MMAN_H
#define __MMAN_H

#include "types.h"

#define PROT_NONE   0x00
#define PROT_READ   0x01
#define PROT_WRITE  0x02
#define PROT_EXEC   0x04

#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02

#define MAP_FIXED   0x10

#define MAP_FAILED ((void *)-1)

#define MAP_FILE        0x00
#ifdef __DARWIN__
#  define MAP_ANON      0x1000
#else
#  define MAP_ANONYMOUS 0x20
#  define MAP_ANON      MAP_ANONYMOUS
#endif

void *mmap(void *addr, size_t len,
  int prot, int flags,
  int fd, off_t offset);

int munmap(void *addr, size_t len);

#endif
