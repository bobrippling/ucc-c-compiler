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

data_store *data_store_new(char *s, int l)
{
	data_store *ds = umalloc(sizeof *ds);

	ds->str = s;
	ds->len = l;

	return ds;
}

void data_store_declare(data_store *ds)
{
	/* only chars for the moment */
	asm_out_section(SECTION_DATA, "%s:\n.byte ", ds->lbl);
}

void data_store_out(data_store *ds, int newline)
{
	int i;
	const char *pre = "";

	for(i = 0; i < ds->len; i++){
		asm_out_section(SECTION_DATA,
				"%s%d", pre, ds->str[i]);
		pre = ", ";
	}

	if(newline)
		asm_out_section(SECTION_DATA, "\n");
}

void data_store_fold_type(data_store *ds, type_ref **ptree_type, symtable *stab)
{
	type_ref *tree_type;
	expr *sz = expr_new_val(ds->len);
	type *type;

	FOLD_EXPR(sz, stab);

	/* (const char []) */
	tree_type = type_ref_new_array(
			type_ref_new_type(
				type = type_new_primitive_qual(type_char, qual_const)),
			sz);

	ds->lbl = out_label_data_store(1);

	*ptree_type = tree_type;
}
