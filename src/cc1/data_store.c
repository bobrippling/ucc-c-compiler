#include <stdarg.h>
#include <stdio.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "data_store.h"
#include "str.h"
#include "cc1.h"
#include "out/asm.h"
#include "out/lbl.h"
#include "fold.h"
#include "decl_init.h"

data_store *data_store_new_str(char *s, int l)
{
	data_store *ds = umalloc(sizeof *ds);

	ds->type     = data_store_str;
	ds->bits.str = s;
	ds->len      = l;

	return ds;
}

void data_store_declare(data_store *ds)
{
	/* only chars for the moment */
	asm_out_section(SECTION_DATA, "%s:\n.byte ", ds->spel);
}

void data_store_out(data_store *ds, int newline)
{
	int i;

	switch(ds->type){
		case data_store_str:
		{
			const char *pre = "";

			for(i = 0; i < ds->len; i++){
				asm_out_section(SECTION_DATA,
						"%s%d", pre, ds->bits.str[i]);
				pre = ", ";
			}

			if(newline)
				asm_out_section(SECTION_DATA, "\n");

			break;
		}
	}
}

void data_store_fold_type(data_store *ds, type_ref **ptree_type, symtable *stab)
{
	type_ref *tree_type;
	expr *sz = expr_new_val(ds->len);
	type *type;

	FOLD_EXPR(sz, stab);

	/* (const char []) */

	tree_type = type_ref_new_array(type_ref_new_type(type = type_new()), sz);

	switch(ds->type){
		case data_store_str:
			type->primitive = type_char;
			type->qual |= qual_const; /* "" is a string constant */
			break;
	}

	ds->spel = out_label_data_store(ds->type == data_store_str);

	*ptree_type = tree_type;
}
