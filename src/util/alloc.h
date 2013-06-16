#ifndef ALLOC_H
#define ALLOC_H

void *umalloc(size_t);
void *urealloc1(void *, size_t new);
void *urealloc(void *, size_t new, size_t old);
char *ustrdup(const char *);
char *ustrdup2(const char *, const char *b); /* up to, but not including b */
char *ustrprintf( const char *, ...);
char *ustrvprintf(const char *, va_list);

#endif
