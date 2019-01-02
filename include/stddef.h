#ifndef _UCC_STDDEF_H
#define _UCC_STDDEF_H

#define NULL (void *)0

#define offsetof(T, m) (unsigned long)&(((T *)0)->m) /*__builtin_offsetof(T, m)*/

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;

typedef int wchar_t;

#endif
