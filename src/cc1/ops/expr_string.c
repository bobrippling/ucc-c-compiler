#include <string.h>

#include "../../util/dynarray.h"
#include "../../util/platform.h"

#include "ops.h"
#include "expr_string.h"
#include "../decl_init.h"
#include "../str.h"
#include "../out/lbl.h"
#include "../type_nav.h"

#include "expr_string.h"
#include "expr_val.h"

const char *str_expr_str(void)
{
	return "string";
}

void fold_expr_str(expr *e, symtable *stab)
{
	const stringlit *const strlit = e->bits.strlit.lit_at.lit;
	expr *sz;

	sz = expr_new_val(strlit->cstr->count);
	FOLD_EXPR(sz, stab);

	/* (const? char []) */
	e->tree_type = type_array_of(
			type_qualify(
				type_nav_btype(
					cc1_type_nav,
					strlit->cstr->type == CSTRING_WIDE ? type_wchar : type_nchar),
				e->bits.strlit.is_func ? qual_const : qual_none),
			sz);
}

const out_val *gen_expr_str(const expr *e, out_ctx *octx)
{
	/* gen_expr_str :: char (*)[]
	 *
	 * just like char x[] :: x vs &x
	 */
	stringlit *strl = e->bits.strlit.lit_at.lit;

	stringlit_use(strl);

	return out_new_lbl(
			octx,
			type_ptr_to(e->tree_type),
			strl->lbl,
			OUT_LBL_PIC | OUT_LBL_PICLOCAL);
}

void dump_expr_str(const expr *e, dump *ctx)
{
	stringlit *lit = e->bits.strlit.lit_at.lit;

	dump_desc_expr_newline(ctx, "string literal", e, 0);

	dump_printf_indent(
			ctx, 0,
			" %sstr ",
			lit->cstr->type == CSTRING_WIDE ? "wide " : "");

	dump_strliteral_indent(
			ctx,
			0,
			e->bits.strlit.lit_at.lit->cstr);
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
	e->f_islval = expr_is_lval_always;
}

void expr_mutate_str(
		expr *e,
		struct cstring *str,
		where *w, symtable *stab)
{
	expr_mutate_wrapper(e, str);

	e->bits.strlit.lit_at.lit = strings_lookup(
			&symtab_global(stab)->literals,
			str);

	memcpy_safe(&e->bits.strlit.lit_at.where, w);
	memcpy_safe(&e->where, w);
}

expr *expr_new_str(struct cstring *str, where *w, symtable *stab)
{
	expr *e = expr_new_wrapper(str);
	expr_mutate_str(e, str, w, stab);
	return e;
}

const out_val *gen_expr_style_str(const expr *e, out_ctx *octx)
{
	extern FILE *cc_out[];

	literal_print(cc_out[0], e->bits.strlit.lit_at.lit->cstr);

	UNUSED_OCTX();
}
