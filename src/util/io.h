#ifndef IO_H
#define IO_H

#include "compiler.h"

char *fline(FILE *f, int *const newline);

int cat(FILE *from, FILE *to);

void xfprintf(FILE *, const char *, ...)
	ucc_printflike(2, 3);

#endif
