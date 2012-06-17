#ifndef DATA_STORE_H
#define DATA_STORE_H

data_store *data_store_new_str(char *s, int l);
void data_store_fold_decl(data_store *ds, decl **ptree_type);

void data_store_out(    data_store *, FILE *);
void data_store_declare(data_store *, FILE *);

#endif
