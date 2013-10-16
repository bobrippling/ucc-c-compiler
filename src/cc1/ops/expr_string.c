#include <string.h>

#include "ops.h"
#include "expr_string.h"
#include "../decl_init.h"
#include "../str.h"
#include "../out/lbl.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"

const char *str_expr_str(void)
{
	return "string";
}

void fold_expr_str(expr *e, symtable *stab)
{
	stringval *const sv = &e->bits.str.sv;
	expr *sz;
	decl *d;
	unsigned i;

	if(e->code)
		return; /* called from a sub-assignment */

	sz = expr_new_val(sv->len);
	FOLD_EXPR(sz, stab);

	/* (const char []) */
	e->tree_type = type_ref_new_array(
			type_ref_new_type_qual(
				sv->wide ? type_wchar : type_nchar,
				qual_const),
			sz);

	sv->lbl = out_label_data_store(1);

	d = decl_new();
	d->ref = e->tree_type;
	d->spel_asm = sv->lbl;

	d->store = store_static;

	d->init = decl_init_new(decl_init_brace);
	for(i = 0; i < sv->len; i++){
		decl_init *di = decl_init_new(decl_init_scalar);

		di->bits.expr = expr_new_val(sv->str[i]);

		dynarray_add(&d->init->bits.ar.inits, di);
	}

	/* add a sym so the data store gets gen'd */
	e->bits.str.sym = sym_new_stab(
			stab,
			d,
			stab->parent ? sym_local : sym_global);

	decl_init_brace_up_fold(d, stab);

	/* no non-global folding,
	 * all strks are static globals/read from the init */
	fold_decl_global_init(d, stab);
}

void gen_expr_str(expr *e)
{
	/*gen_asm_local(e->bits.str.sym.decl); - done for the decl we create */
	out_push_lbl(e->bits.str.sv.lbl, 1);
	out_set_lvalue();
}

void gen_expr_str_str(expr *e)
{
	stringval *sv = &e->bits.str.sv;
	FILE *f = gen_file();

	idt_printf("%sstring at %s\n", sv->wide ? "wide " : "", sv->lbl);
	gen_str_indent++;
	idt_print();
	literal_print(f, e->bits.str.sv.str, e->bits.str.sv.len);
	gen_str_indent--;
	fputc('\n', f);
}

static void const_expr_string(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	if(e->bits.str.sv.wide){
		k->type = CONST_NO;
		ICW("TODO: wide string const");
	}else{
		k->type = CONST_STRK;
		k->bits.str = &e->bits.str.sv;
		k->offset = 0;
	}
}

void mutate_expr_str(expr *e)
{
	e->f_const_fold = const_expr_string;
}

void expr_mutate_str(expr *e, char *s, int len)
{
	stringval *sv = &e->bits.str.sv;

	expr_mutate_wrapper(e, str);

	memset(sv, 0, sizeof *sv);
	sv->str = s;
	sv->len = len;
}

expr *expr_new_str(char *s, int l, int wide)
{
	expr *e = expr_new_wrapper(str);
	expr_mutate_str(e, s, l);
	e->bits.str.sv.wide = wide;
	memcpy_safe(&e->bits.str.sv.where, &e->where);
	return e;
}

void gen_expr_style_str(expr *e)
{
	literal_print(gen_file(), e->bits.str.sv.str, e->bits.str.sv.len);
}
