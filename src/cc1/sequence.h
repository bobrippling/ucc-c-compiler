#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "../util/compiler.h"

/* which symtable is unimportant, so long as we can get to the global symtable */

void sequence_read(expr *, sym *, symtable *)
	ucc_nonnull((1, 3));
void sequence_write(expr *, sym *, symtable *)
	ucc_nonnull((1, 3));
void sequence_point(symtable *);

enum sym_rw sequence_state(expr *, sym *, symtable *)
	ucc_nonnull((1, 3));
void sequence_set_state(expr *, sym *, symtable *, enum sym_rw)
	ucc_nonnull((1, 3));

#endif
