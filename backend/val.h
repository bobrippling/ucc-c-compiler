#ifndef VAL_H
#define VAL_H

typedef struct val val;

val *val_new_i(int);
val *val_new_ptr_from_int(int);

val *val_alloca(int n, unsigned elemsz);
val *val_element(val *lval, int i, unsigned elemsz); /* i'th element */

void val_store(val *rval, val *lval);
val *val_load(val *);

val *val_add(val *, val *);

char *val_str(val *);

unsigned val_hash(val *);

#endif
