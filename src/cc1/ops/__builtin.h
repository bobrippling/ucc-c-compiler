#ifndef __BUILTIN_H
#define __BUILTIN_H

expr *builtin_parse(const char *sp);

expr *builtin_new_memset(expr *p, int ch, size_t len);
expr *builtin_new_memcpy(expr *to, expr *from, size_t len);

#endif
