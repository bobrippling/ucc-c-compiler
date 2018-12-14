#ifndef IO_H
#define IO_H

char *fline(FILE *f, int *const newline);

int cat(FILE *from, FILE *to);

void xfprintf(FILE *, const char *, ...);

#endif
