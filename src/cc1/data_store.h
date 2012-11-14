#ifndef DATA_STORE_H
#define DATA_STORE_H

struct data_store
{
	enum
	{
		data_store_str
	} type;

	union
	{
		char *str;
	} bits;
	int len;

	char *spel; /* asm */
};

data_store *data_store_new_str(char *s, int l);
void data_store_fold_type(data_store *ds, type_ref **ptree_type, symtable *stab);

void data_store_out(    data_store *, int newline);
void data_store_declare(data_store *);

#endif
