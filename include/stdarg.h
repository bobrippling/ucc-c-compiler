#ifndef __UCC_STDARG_H
#define __UCC_STDARG_H

#ifdef __x86_64__
typedef __builtin_va_list va_list;

#  define va_start(l, arg) __builtin_va_start(l, arg)
#  define va_arg(l, type)  __builtin_va_arg(l, type)
#  define va_end(l)        __builtin_va_end(l)
#  define va_copy(d, s)    __builtin_va_copy(d, s)
#else
#  warning untested i386 va_arg

typedef char *va_list;

#  define va_start(l, arg) ((l) = (char *)&(arg))
#  define va_arg(l, type) (*(type *)((l) += 4))
#  define va_end(l)
#  define va_copy(d, s) ((d) = (s))
#endif

#endif
