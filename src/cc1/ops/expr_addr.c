#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ops.h"
#include "expr_addr.h"

#include "../out/lbl.h"
#include "../label.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "expr_identifier.h"
#include "expr_struct.h"

const char *str_expr_addr(void)
{
	return "address-of";
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
			in_func->bits.func.contains_static_label_addr = 1;

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
					expr_str_friendly(e->lhs, 0), type_to_str(e->lhs->tree_type));
			fold_had_error = 1;
			return;
		}

		if(expr_kind(e->lhs, identifier)){
			sym *sym = e->lhs->bits.ident.bits.ident.sym;
			if(sym){
				decl *d = sym->decl;

				if((d->store & STORE_MASK_STORE) == store_register){
					warn_at_print_error(&e->lhs->where, "can't take the address of register");
					fold_had_error = 1;
				}

				d->flags |= DECL_FLAGS_ADDRESSED;
			}
		}else if(expr_kind(e->lhs, struct)){
			decl *d = e->lhs->bits.struct_mem.d;
			type *suty = e->lhs->lhs->tree_type;
			struct_union_enum_st *su = type_is_s_or_u(
					type_is_ptr(suty) ? type_is_ptr(suty) : suty);
			int attr_on_decl;

			assert(su);
			if((attr_on_decl = !!attribute_present(d, attr_packed)) || attr_present(su->attr, attr_packed)){
				const int warned = cc1_warn_at(&e->where, address_of_packed, "taking the address of a packed member");

				if(warned)
					note_at((attr_on_decl ? &d->where : &su->where), "declared here");
			}
		}

		if(fold_check_expr(e->lhs,
					FOLD_CHK_ALLOW_VOID | FOLD_CHK_NO_BITFIELD,
					"address-of"))
		{
			return;
		}
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

void dump_expr_addr(const expr *e, dump *ctx)
{
	if(e->bits.lbl.spel){
		dump_desc_expr(ctx, "label address", e);
		dump_inc(ctx);
		dump_strliteral(ctx, e->bits.lbl.spel, strlen(e->bits.lbl.spel));
		dump_dec(ctx);
	}else{
		dump_desc_expr(ctx, "address-of", e);
		dump_inc(ctx);
		dump_expr(e->lhs, ctx);
		dump_dec(ctx);
	}
}

static void const_expr_addr(expr *e, consty *k)
{
	if(e->bits.lbl.spel){
		int static_ctx = e->bits.lbl.static_ctx; /* global or static */

		/*k->sym_lbl = e->bits.lbl.spel;*/
		CONST_FOLD_LEAF(k);
		k->type = CONST_ADDR;
		k->offset = 0;
		k->bits.addr.lbl_type = CONST_LBL_TRUE;

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
				CONST_FOLD_NO(k, e);
				break;
		}
	}
}

static int expr_addr_has_sideeffects(const expr *e)
{
	return e->lhs && expr_has_sideeffects(e->lhs);
}

static int expr_addr_requires_relocation(const expr *e)
{
	return e->lhs && expr_requires_relocation(e->lhs);
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
	e->f_has_sideeffects = expr_addr_has_sideeffects;
	e->f_requires_relocation = expr_addr_requires_relocation;
}

const out_val *gen_expr_style_addr(const expr *e, out_ctx *octx)
{
	const out_val *r;
	stylef("&(");
	r = gen_expr(e->lhs, octx);
	stylef(")");
	return r;
}
