#ifndef __STDARG_H
#define __STDARG_H

#include "sys/types.h"

#ifdef __TYPEDEFS_WORKING
typedef void *va_list;
#else
#define va_list void *
#endif

#define va_start(l, arg) l = (void *)&arg
/*
 * va_arg assumes arguments are promoted to
 * machine-word size when pushed onto the stack
 */
#define va_arg(l, type) (*(type *)(l += sizeof *l))
#define va_end(l)

#endif
