#ifndef _UCC_STDARG_H
#define _UCC_STDARG_H

typedef __builtin_va_list va_list;

#define va_start(l, arg) __builtin_va_start(l, arg)
#define va_arg(l, type)  __builtin_va_arg(l, type)
#define va_end(l)        __builtin_va_end(l)

#if __STDC_VERSION__ >= 199900L
#  define va_copy(d, s)    __builtin_va_copy(d, s)
#endif

#define __va_copy(d, s) __builtin_va_copy(d, s)

#endif
