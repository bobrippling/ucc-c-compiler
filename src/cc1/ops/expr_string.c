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
	const stringlit *const strlit = e->bits.strlit.lit_at.lit;
	expr *sz;

	sz = expr_new_val(strlit->len);
	FOLD_EXPR(sz, stab);

	/* (const? char []) */
	e->tree_type = type_ref_new_array(
			type_ref_new_type_qual(
				strlit->wide ? type_wchar : type_char,
				e->bits.strlit.is_func ? qual_const : qual_none),
			sz);
}

void gen_expr_str(expr *e)
{
	stringlit_use(e->bits.strlit.lit_at.lit);
	out_push_lbl(e->bits.strlit.lit_at.lit->lbl, 1);
}

static void lea_expr_str(expr *e)
{
	/* looks the same - a lea, but the type is different
	 * gen_expr_str :: char *
	 * lea_expr_str :: char (*)[]
	 *
	 * just like char x[] :: x vs &x
	 */
	gen_expr_str(e);
}

void gen_expr_str_str(expr *e)
{
	stringlit *lit = e->bits.strlit.lit_at.lit;

	idt_printf("%sstring at %s\n", lit->wide ? "wide " : "", lit->lbl);
	gen_str_indent++;
	idt_print();

	literal_print(cc1_out,
			e->bits.strlit.lit_at.lit->str,
			e->bits.strlit.lit_at.lit->len);

	gen_str_indent--;
	fputc('\n', cc1_out);
}

static void const_expr_string(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	if(e->bits.strlit.lit_at.lit->wide){
		k->type = CONST_NO;
		ICW("TODO: wide string const");
	}else{
		k->type = CONST_STRK;
		k->bits.str = &e->bits.strlit.lit_at;
		k->offset = 0;
	}
}

void mutate_expr_str(expr *e)
{
	e->f_const_fold = const_expr_string;
	e->f_lea = lea_expr_str;
}

void expr_mutate_str(
		expr *e,
		char *s, size_t len,
		int wide,
		where *w)
{
	expr_mutate_wrapper(e, str);

	e->bits.strlit.lit_at.lit = strings_lookup(
			&symtab_global(current_scope)->literals,
			s, len, wide);

	memcpy_safe(&e->bits.strlit.lit_at.where, w);
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
			e->bits.strlit.lit_at.lit->str,
			e->bits.strlit.lit_at.lit->len);
}
