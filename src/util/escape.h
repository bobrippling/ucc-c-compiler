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

/* *eptr = NULL, on error */
unsigned long long char_seq_to_ullong(
		char *s,
		char **eptr,
		enum base mode,
		int *of)
	ucc_nonnull();

long read_quoted_char(
		char *start, char **end,
		int *multichar, int clip_256,
		const char **const err,
		int *const warn)
	ucc_nonnull();

/* *warn is set to ERANGE if a large sequence is parsed,
 * or EINVAL if invalid escape char */
long read_char_single(
		char *start, char **end, int *const warn) ucc_nonnull();

#define isoct(x) ('0' <= (x) && (x) < '8')

#endif
