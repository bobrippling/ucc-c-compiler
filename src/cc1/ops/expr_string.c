#include "ops.h"
#include "expr_string.h"
#include "../decl_init.h"
#include "../str.h"

const char *str_expr_str(void)
{
	return "string";
}

void fold_expr_str(expr *e, symtable *stab)
{
	decl *d_str;
	int i;
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

	d_str = e->val.decl = decl_new();
	d_str->ref = e->tree_type;
	d_str->spel = e->data_store->lbl;

	d_str->is_definition = 1;
	d_str->store = store_static;

	d_str->init = decl_init_new(decl_init_brace);
	for(i = 0; i < e->data_store->len; i++){
		expr *a = expr_new_assign(
				expr_new_array_idx(
				);

	}

	/* add a sym so the data store gets gen'd */
	SYMTAB_ADD(stab, d_str, stab->parent ? sym_local : sym_global);
	/* don't set e->sym */
}

void gen_expr_str(expr *e, symtable *stab)
{
	(void)stab;
	out_push_lbl(e->data_store->lbl, 1);
}

void gen_expr_str_str(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("address of datastore %s\n", e->data_store->lbl);
	gen_str_indent++;
	idt_print();
	literal_print(cc1_out, e->data_store->str, e->data_store->len);
	gen_str_indent--;
	fputc('\n', cc1_out);
}

void const_expr_string(expr *e, consty *k)
{
	k->type = CONST_STRK;
	k->bits.str = e->data_store;
	k->offset = 0;
}

void mutate_expr_str(expr *e)
{
	e->f_const_fold = const_expr_string;
}

void expr_mutate_str(expr *e, char *s, int len)
{
	e->data_store = data_store_new(s, len);
}

expr *expr_new_str(char *s, int l)
{
	expr *e = expr_new_wrapper(str);
	expr_mutate_str(e, s, l);
	return e;
}

void gen_expr_style_str(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
