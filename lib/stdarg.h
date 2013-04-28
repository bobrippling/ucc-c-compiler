#ifndef __STDARG_H
#define __STDARG_H

#include "sys/types.h"

typedef char *va_list;

#define va_start(l, arg) l = (va_list)&arg
/*
 * va_arg assumes arguments are promoted to
 * machine-word size when pushed onto the stack
 */
#define va_arg(l, type) (*(type *)(l += __WORDSIZE / 8))
#define va_end(l)

#endif
