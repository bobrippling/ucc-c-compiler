#ifndef __STDARG_H
#define __STDARG_H

#include "sys/types.h"

typedef void *va_list;

#define va_start(l, arg) l = (void *)&arg
#define va_arg(l, type) (*(type *)(l += __WORDSIZE / 8))
#define va_end(l)

#endif
