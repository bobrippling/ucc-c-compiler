#include <stdarg.h>
#include <stdio.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "data_store.h"

static const char *ds_nams[] = { "str" };

#define data_store_to_str(x) ds_nams[x->type]


data_store *data_store_new_str(char *s, int l)
{
	data_store *ds = umalloc(sizeof *ds);

	ds->type     = data_store_str;
	ds->data.str = s;
	ds->len      = l;

	return ds;
}

void data_store_out(FILE *f, data_store *ds)
{
	switch(ds->type){
		case data_store_str:;
	}

	fprintf(f, "; TODO: data store %s", data_store_to_str(ds));
}
