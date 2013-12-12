#ifndef UCC_H
#define UCC_H

extern const char *argv0;
extern char *wrapper; /* -wrapper gdb,--args */
extern char *Bprefix; /* -Bexec-prefix */
void die(const char *s, ...);

#endif
