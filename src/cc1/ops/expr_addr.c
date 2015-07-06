#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ops.h"
#include "expr_addr.h"

#include "../out/lbl.h"
#include "../label.h"
#include "../type_is.h"
#include "../type_nav.h"

const char *str_expr_addr()
{
	return "addr";
}

int expr_is_addressable(expr *e)
{
	if(type_is(e->tree_type, type_func))
		return 1;

	return expr_is_lval(e) == LVALUE_USER_ASSIGNABLE;
}

expr *expr_addr_target(const expr *e)
{
	if(e->bits.lbl.spel)
		return NULL;

	return e->lhs;
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->bits.lbl.spel){
		decl *in_func = symtab_func(stab);

		if(!in_func)
			die_at(&e->where, "address-of-label outside a function");

		if(e->bits.lbl.static_ctx)
			in_func->bits.func.contains_addr_lbl = 1;

		(e->bits.lbl.label =
		 symtab_label_find_or_new(
			 stab, e->bits.lbl.spel, &e->where))
			->uses++;

		/* address of label - void * */
		e->tree_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));

	}else{
		/* if it's an identifier, act as a read */
		fold_inc_writes_if_sym(e->lhs, stab);

		fold_expr_nodecay(e->lhs, stab);

		e->tree_type = type_ptr_to(e->lhs->tree_type);

		/* can address: lvalues, arrays and functions */
		if(!expr_is_addressable(e->lhs)){
			warn_at_print_error(&e->where, "can't take the address of %s (%s)",
					e->lhs->f_str(), type_to_str(e->lhs->tree_type));
			fold_had_error = 1;
			return;
		}

		if(expr_kind(e->lhs, identifier)){
			decl *d = e->lhs->bits.ident.bits.ident.sym->decl;

			if((d->store & STORE_MASK_STORE) == store_register)
				die_at(&e->lhs->where, "can't take the address of register");
		}

		fold_check_expr(e->lhs, FOLD_CHK_ALLOW_VOID | FOLD_CHK_NO_BITFIELD,
				"address-of");
	}
}

const out_val *gen_expr_addr(const expr *e, out_ctx *octx)
{
	if(e->bits.lbl.spel){
		/* GNU &&lbl */
		out_blk *blk = label_getblk(e->bits.lbl.label, octx);

		return out_new_blk_addr(octx, blk);

	}else{
		/* gen_expr works, even for &expr, because the fold_expr_no_decay()
		 * means we don't lval2rval our sub-expression */
		return gen_expr(e->lhs, octx);
	}
}

const out_val *gen_expr_str_addr(const expr *e, out_ctx *octx)
{
	if(e->bits.lbl.spel){
		idt_printf("address of label \"%s\"\n", e->bits.lbl.spel);
	}else{
		idt_printf("address of expr:\n");
		gen_str_indent++;
		print_expr(e->lhs);
		gen_str_indent--;
	}
	UNUSED_OCTX();
}

static void const_expr_addr(expr *e, consty *k)
{
	if(e->bits.lbl.spel){
		int static_ctx = e->bits.lbl.static_ctx; /* global or static */

		/*k->sym_lbl = e->bits.lbl.spel;*/
		CONST_FOLD_LEAF(k);
		k->type = CONST_ADDR;
		k->offset = 0;
		k->bits.addr.is_lbl = 1;

		if(static_ctx){
			e->bits.lbl.label->mustgen_spel = out_label_code("goto");

			k->bits.addr.bits.lbl = e->bits.lbl.label->mustgen_spel;
		}else{
			k->bits.addr.bits.lbl = e->bits.lbl.label->spel;
		}

	}else{
		const_fold(e->lhs, k);

		switch(k->type){
			case CONST_NEED_ADDR:
				/* it's a->b, a symbol, etc */
				k->type = CONST_ADDR; /* addr is const but with no value */
				break;

			case CONST_STRK:
			case CONST_ADDR:
				/* int x[]; int *p = &x;
				 * already addr, just roll with it.
				 * lvalue/etc checks are done in fold
				 */
				break;

			default:
				k->type = CONST_NO;
				break;
		}
	}
}

expr *expr_new_addr(expr *sub)
{
	expr *e = expr_new_wrapper(addr);
	e->lhs = sub;
	return e;
}

expr *expr_new_addr_lbl(char *lbl, int static_ctx)
{
	expr *e = expr_new_wrapper(addr);
	e->bits.lbl.spel = lbl;
	e->bits.lbl.static_ctx = static_ctx;
	return e;
}

void mutate_expr_addr(expr *e)
{
	e->f_const_fold = const_expr_addr;
}

const out_val *gen_expr_style_addr(const expr *e, out_ctx *octx)
{
	const out_val *r;
	stylef("&(");
	r = gen_expr(e->lhs, octx);
	stylef(")");
	return r;
}
