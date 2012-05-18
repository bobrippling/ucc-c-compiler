#ifndef DATA_STORE_H
#define DATA_STORE_H

data_store *data_store_new_str(char *s, int l);
void data_store_out(FILE *f, data_store *);
void data_store_fold_decl(data_store *ds, decl **ptree_type);

#endif
