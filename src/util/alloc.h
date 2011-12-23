#ifndef ALLOC_H
#define ALLOC_H

void *umalloc(size_t);
void *urealloc(void *, size_t);
char *ustrdup(const char *);
char *ustrprintf(const char *, ...);

#endif
