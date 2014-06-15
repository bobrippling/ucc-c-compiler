#ifndef ESCAPE_H
#define ESCAPE_H

#include "compiler.h"

enum base
{
	BIN, OCT, DEC, HEX
};

const char *base_to_str(enum base);

/* -1 on error */
int escape_char(int c);

/* these set *eptr to NULL on error */
long escape_multi_char(char *pos, char **eptr) ucc_nonnull();

unsigned long long char_seq_to_ullong(
		char *s, char **eptr, enum base mode,
		int limit, /* limit to 3 for octal? */
		int *of)
	ucc_nonnull();

/* we're just after the opening quote */
long read_quoted_char(
		char *start, char **end,
		int *multichar) ucc_nonnull();

/* we're in the quote, single or double */
long read_char_single(char *start, char **end, unsigned off) ucc_nonnull();

#define isoct(x) ('0' <= (x) && (x) < '8')

#endif
