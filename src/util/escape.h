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

long char_seq_to_long(
		char *s, char **eptr, enum base mode) ucc_nonnull();

#define isoct(x) ('0' <= (x) && (x) < '8')

#endif
