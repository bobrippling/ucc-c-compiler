#include "ops.h"
#include "expr_string.h"
#include "../decl_init.h"
#include "../str.h"
#include "../out/lbl.h"
#include "../../util/dynarray.h"

const char *str_expr_str(void)
{
	return "string";
}

void fold_expr_str(expr *e, symtable *stab)
{
	expr *sz;
	decl *d;
	type *type;
	unsigned i;

	if(e->code)
		return; /* called from a sub-assignment */

	sz = expr_new_val(e->bits.str.sv.len);
	FOLD_EXPR(sz, stab);

	/* (const char []) */
	e->tree_type = type_ref_new_array(
			type_ref_new_type(
				type = type_new_primitive_qual(type_char, qual_const)),
			sz);

	e->bits.str.sv.lbl = out_label_data_store(1);

	d = decl_new();
	d->ref = e->tree_type;
	d->spel = e->bits.str.sv.lbl;

	d->is_definition = 1;
	d->store = store_static;

	d->init = decl_init_new(decl_init_brace);
	for(i = 0; i < e->bits.str.sv.len; i++){
		decl_init *di = decl_init_new(decl_init_scalar);

		di->bits.expr = expr_new_val(e->bits.str.sv.str[i]);

		dynarray_add((void ***)&d->init->bits.inits, di);
	}

	/* add a sym so the data store gets gen'd */
	e->bits.str.sym = SYMTAB_ADD(stab, d, stab->parent ? sym_local : sym_global);

	e->code = stmt_new_wrapper(code, stab);
	decl_init_create_assignments_for_base(d, e, e->code);

	/* no non-global folding,
	 * all strks are static globals/read from the init */
	fold_decl_global_init(d, stab);
}

void gen_expr_str(expr *e, symtable *stab)
{
	(void)stab;
	out_push_lbl(e->bits.str.sv.lbl, 1);
}

void gen_expr_str_str(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("address of datastore %s\n", e->bits.str.sv.lbl);
	gen_str_indent++;
	idt_print();
	literal_print(cc1_out, e->bits.str.sv.str, e->bits.str.sv.len);
	gen_str_indent--;
	fputc('\n', cc1_out);
}

void const_expr_string(expr *e, consty *k)
{
	k->type = CONST_STRK;
	k->bits.str = &e->bits.str.sv;
	k->offset = 0;
}

void mutate_expr_str(expr *e)
{
	e->f_const_fold = const_expr_string;
}

void expr_mutate_str(expr *e, char *s, int len)
{
	stringval *sv = &e->bits.str.sv;

	expr_mutate_wrapper(e, str);

	sv->str = s;
	sv->len = len;
}

expr *expr_new_str(char *s, int l)
{
	expr *e = expr_new_wrapper(str);
	expr_mutate_str(e, s, l);
	return e;
}

void gen_expr_style_str(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
