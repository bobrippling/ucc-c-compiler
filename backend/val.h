#ifndef VAL_H
#define VAL_H

typedef struct val val;

val *val_new_i(int);
val *val_new_ptr_from_int(int);

val *val_alloca(void);

void val_store(val *rval, val *lval);
val *val_load(val *);

val *val_add(val *, val *);

char *val_str(val *);

unsigned val_hash(val *);

#endif
