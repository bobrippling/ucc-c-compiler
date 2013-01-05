#ifndef DATA_STORE_H
#define DATA_STORE_H

struct data_store
{
	char *lbl; /* asm */
	char *str;
	int len;
};

data_store *data_store_new(char *s, int l);
void data_store_fold_type(data_store *ds, type_ref **ptree_type, symtable *stab);

void data_store_out(    data_store *, int newline);
void data_store_declare(data_store *);

#endif
