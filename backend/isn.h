#ifndef ISN_H
#define ISN_H

#include "op.h"

void isn_load(val *to, val *lval);
void isn_store(val *from, val *lval);

void isn_alloca(unsigned sz, val *);

void isn_op(enum op op, val *lhs, val *rhs, val *res);
void isn_elem(val *lval, val *add, val *res);

void isn_dump(void);

#endif
