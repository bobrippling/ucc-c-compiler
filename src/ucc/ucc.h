#ifndef UCC_H
#define UCC_H

#include "../util/compiler.h"

extern const char *argv0;
extern char *wrapper; /* -wrapper gdb,--args */
extern char *Bprefix; /* -Bexec-prefix */
void die(const char *s, ...) ucc_printflike(1, 2);

#endif
