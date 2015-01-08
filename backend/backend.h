#ifndef BACKEND_H
#define BACKEND_H

typedef struct val val;

val *val_new_i(int);
val *val_new_ptr_from_int(int);
void val_store(val *, val *);
val *val_load(val *);
val *val_add(val *, val *);
val *val_show(val *);

#endif
