#include <string.h>
#include <assert.h>

#include "ops.h"
#include "expr_deref.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../str.h"

#include "expr_op.h"

const char *str_expr_deref()
{
	return "dereference";
}

void fold_expr_deref(expr *e, symtable *stab)
{
	expr *ptr;

	ptr = FOLD_EXPR(expr_deref_what(e), stab);

	if(!type_is_ptr(ptr->tree_type)){
		fold_had_error = 1;
		warn_at_print_error(&ptr->where,
				"indirection applied to '%s'",
				type_to_str(ptr->tree_type));
		e->tree_type = ptr->tree_type;
		return;
	}

	if(expr_attr_present(ptr, attr_noderef))
		cc1_warn_at(&ptr->where, attr_noderef,
				"dereference of noderef expression");

	fold_check_bounds(ptr, 0);

	e->tree_type = type_is_ptr(ptr->tree_type);
}

const out_val *gen_expr_deref(const expr *e, out_ctx *octx)
{
	/* lea - we're an lvalue */
	return gen_expr(expr_deref_what(e), octx);
}

void dump_expr_deref(const expr *e, dump *ctx)
{
	expr *what = expr_deref_what(e);

	if(expr_kind(what, op) && what->bits.op.array_notation){
		dump_desc_expr(ctx, "array subscript", e);
		dump_inc(ctx);
		dump_expr(what->lhs, ctx);
		dump_expr(what->rhs, ctx);
		dump_dec(ctx);
	}else{
		dump_desc_expr(ctx, "dereference", e);
		dump_inc(ctx);
		dump_expr(what, ctx);
		dump_dec(ctx);
	}
}

static void const_expr_deref(expr *e, consty *k)
{
	expr *from = expr_deref_what(e);

	const_fold(from, k);

	switch(k->type){
		case CONST_STRK:
		{
			stringlit *sv = k->bits.str->lit;

			/* check type we're supposed to be dereferencing as,
			 * should be char *
			 */
			if(!type_is_primitive_anysign(type_is_ptr(from->tree_type), type_nchar)){
				CONST_FOLD_NO(k, e);
				break;
			}

			if(k->offset < 0 || (unsigned)k->offset >= sv->cstr->count){
				/* undefined - we define as */
				CONST_FOLD_NO(k, e);
			}else{
				const long offset = k->offset;

				switch(sv->cstr->type){
					case CSTRING_ASCII:
						/* need to preserve original string for lvalue-ness -> CONST_NEED_ADDR */
						CONST_FOLD_LEAF(k);
						k->type = CONST_NEED_ADDR;
						k->bits.addr.lbl_type = CONST_LBL_TRUE;
						k->bits.addr.bits.lbl = sv->lbl;
						k->offset = offset;
						break;
					case CSTRING_RAW:
						assert(0 && "raw string in code gen");
					case CSTRING_WIDE:
						assert(0 && "TODO: wide string gen");
				}

				stringlit_use(sv); /* ensure emit */
			}
			break;
		}
		case CONST_NEED_ADDR:
			CONST_FOLD_NO(k, e);
			break;

		case CONST_NUM:
		{
			integral_t num = k->bits.num.val.i;
			CONST_FOLD_LEAF(k);
			k->bits.addr.lbl_type = CONST_LBL_MEMADDR;
			k->bits.addr.bits.memaddr = num;
		} /* fall */
		case CONST_ADDR:
			/* *(int [10])a -> still need_addr */
			k->type = CONST_ADDR_OR_NEED_TYPE(e->tree_type);
			break;
		case CONST_NO:
			break;
	}
}

static int expr_deref_has_sideeffects(const expr *e)
{
	return expr_has_sideeffects(expr_deref_what(e));
}

void mutate_expr_deref(expr *e)
{
	e->f_const_fold = const_expr_deref;

	e->f_islval = expr_is_lval_always;
	e->f_has_sideeffects = expr_deref_has_sideeffects;
}

expr *expr_new_deref(expr *of)
{
	expr *e = expr_new_wrapper(deref);
	expr_deref_what(e) = of;
	return e;
}

const out_val *gen_expr_style_deref(const expr *e, out_ctx *octx)
{
	stylef("*(");
	IGNORE_PRINTGEN(gen_expr(expr_deref_what(e), octx));
	stylef(")");
	return NULL;
}
