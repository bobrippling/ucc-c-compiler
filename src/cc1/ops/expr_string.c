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
	const stringlit *const strlit = e->bits.strlit.lit;
	expr *sz;
	decl *d;
	unsigned i;

	if(e->code)
		return; /* called from a sub-assignment */

	sz = expr_new_val(strlit->len);
	FOLD_EXPR(sz, stab);

	/* (char []) */
	e->tree_type = type_ref_new_array(
			type_ref_new_type_primitive(strlit->wide ? type_wchar : type_char),
			sz);

	d = decl_new();
	d->ref = e->tree_type;
	d->spel_asm = strlit->lbl;

	d->store = store_static;

	d->init = decl_init_new(decl_init_brace);
	for(i = 0; i < strlit->len; i++){
		decl_init *di = decl_init_new(decl_init_scalar);

		di->bits.expr = expr_new_val(strlit->str[i]);

		dynarray_add(&d->init->bits.ar.inits, di);
	}

	decl_init_brace_up_fold(d, stab);

	/* no non-global folding,
	 * all strks are static globals/read from the init */
	fold_decl_global_init(d, stab);
}

void gen_expr_str(expr *e)
{
	stringlit_use(e->bits.strlit.lit);
	out_push_lbl(e->bits.strlit.lit->lbl, 1);
}

void gen_expr_str_str(expr *e)
{
	stringlit *lit = e->bits.strlit.lit;

	idt_printf("%sstring at %s\n", lit->wide ? "wide " : "", lit->lbl);
	gen_str_indent++;
	idt_print();
	literal_print(cc1_out, e->bits.strlit.lit->str, e->bits.strlit.lit->len);
	gen_str_indent--;
	fputc('\n', cc1_out);
}

static void const_expr_string(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	if(e->bits.strlit.lit->wide){
		k->type = CONST_NO;
		ICW("TODO: wide string const");
	}else{
		k->type = CONST_STRK;
		k->bits.str = &e->bits.strlit;
		k->offset = 0;
	}
}

void mutate_expr_str(expr *e)
{
	e->f_const_fold = const_expr_string;
}

void expr_mutate_str(
		expr *e,
		char *s, size_t len,
		int wide,
		where *w)
{
	expr_mutate_wrapper(e, str);

	e->bits.strlit.lit = strings_lookup(
			&symtab_global(current_scope)->literals,
			s, len, wide);

	memcpy_safe(&e->bits.strlit.where, w);
	memcpy_safe(&e->where, w);
}

expr *expr_new_str(char *s, size_t l, int wide, where *w)
{
	expr *e = expr_new_wrapper(str);
	expr_mutate_str(e, s, l, wide, w);
	return e;
}

void gen_expr_style_str(expr *e)
{
	literal_print(cc1_out,
			e->bits.strlit.lit->str,
			e->bits.strlit.lit->len);
}
