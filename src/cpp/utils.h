#ifndef UTILS_H
#define UTILS_H

enum archtype { ARCH_UNKNOWN, ARCH_32, ARCH_64 } getarch(void);

char *readline(FILE *);

#endif
