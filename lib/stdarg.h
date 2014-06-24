#ifndef __STDARG_H
#define __STDARG_H

#include "sys/types.h"

#ifdef __x86_64__
typedef __builtin_va_list va_list;

#  define va_start(l, arg) __builtin_va_start(l, arg)
#  define va_arg(l, type)  __builtin_va_arg(l, type)
#  define va_end(l)        __builtin_va_end(l)
#  define va_copy(d, s)    __builtin_va_copy(d, s)
#else
#  warning untested i386 va_arg

typedef char *va_list;

#  define va_start(l, arg) l = (char *)&arg
/*
 * va_arg assumes arguments are promoted to
 * machine-word size when pushed onto the stack
 */
#  define va_arg(l, type) (*(type *)(l += __WORDSIZE / 8))
#  define va_end(l)
#  define va_copy(d, s) ((d) = (s))
#endif

extern void __clean_va_list(va_list *);
typedef va_list __attribute((cleanup(__clean_va_list))) __cleaned_va_list;

#endif
