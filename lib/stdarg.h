#ifndef __STDARG_H
#define __STDARG_H

typedef void *va_list;

#define va_start(l, arg) l = (void *)&arg + sizeof(void *)
#define va_arg(l, type) (l += sizeof(void *), (type)(((int *)(l)))[-1])
#define va_end(l)

#endif
