#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "../util/compiler.h"

void sequence_read(expr *, sym *, symtable *)
	ucc_nonnull((1, 3));
void sequence_write(expr *, sym *, symtable *)
	ucc_nonnull((1, 3));
void sequence_point(symtable *);

#endif
