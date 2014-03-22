#include <string.h>

#include "../../util/dynarray.h"
#include "../../util/platform.h"

#include "ops.h"
#include "expr_string.h"
#include "../decl_init.h"
#include "../str.h"
#include "../out/lbl.h"
#include "../type_nav.h"

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
	e->tree_type = type_array_of(
			type_qualify(
				type_nav_btype(
					cc1_type_nav,
					strlit->wide ? type_wchar : type_nchar),
				e->bits.strlit.is_func ? qual_const : qual_none),
			sz);
}

out_val *gen_expr_str(expr *e, out_ctx *octx)
{
	stringlit *strl = e->bits.strlit.lit_at.lit;

	stringlit_use(strl);

	return out_new_lbl(octx, strl->lbl, 1);
}

static out_val *lea_expr_str(expr *e, out_ctx *octx)
{
	/* looks the same - a lea, but the type is different
	 * gen_expr_str :: char *
	 * lea_expr_str :: char (*)[]
	 *
	 * just like char x[] :: x vs &x
	 */
	return gen_expr_str(e, octx);
}

out_val *gen_expr_str_str(expr *e, out_ctx *octx)
{
	FILE *f = gen_file();
	stringlit *lit = e->bits.strlit.lit_at.lit;

	idt_printf("%sstring at %s\n", lit->wide ? "wide " : "", lit->lbl);
	gen_str_indent++;
	idt_print();

	literal_print(f,
			e->bits.strlit.lit_at.lit->str,
			e->bits.strlit.lit_at.lit->len);

	gen_str_indent--;
	fputc('\n', f);

	UNUSED_OCTX();
}

static void const_expr_string(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	k->type = CONST_STRK;
	k->bits.str = &e->bits.strlit.lit_at;
	k->offset = 0;
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
		where *w, symtable *stab)
{
	expr_mutate_wrapper(e, str);

	e->bits.strlit.lit_at.lit = strings_lookup(
			&symtab_global(stab)->literals,
			s, len, wide);

	memcpy_safe(&e->bits.strlit.lit_at.where, w);
	memcpy_safe(&e->where, w);
}

expr *expr_new_str(char *s, size_t l, int wide, where *w, symtable *stab)
{
	expr *e = expr_new_wrapper(str);
	expr_mutate_str(e, s, l, wide, w, stab);
	return e;
}

out_val *gen_expr_style_str(expr *e, out_ctx *octx)
{
	literal_print(gen_file(),
			e->bits.strlit.lit_at.lit->str,
			e->bits.strlit.lit_at.lit->len);

	UNUSED_OCTX();
}
