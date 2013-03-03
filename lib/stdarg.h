#ifndef __STDARG_H
#define __STDARG_H

#include "sys/types.h"

typedef void *va_list;

#define va_start(l, arg) l = (void *)&arg
/*
 * va_arg assumes arguments are promoted to
 * machine-word size when pushed onto the stack
 */
#define va_arg(l, type) (*(type *)(l = (char *)l + __WORDSIZE / 8))
#define va_end(l)

#endif
