#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_addr.h"
#include "../out/lbl.h"
#include "../label.h"

const char *str_expr_addr()
{
	return "addr";
}

int expr_is_addressable(expr *e)
{
	return expr_is_lvalue(e)
		|| type_ref_is(e->tree_type, type_ref_array)
		|| type_ref_is(e->tree_type, type_ref_func);
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->bits.lbl.spel){
		if(!curdecl_func)
			die_at(&e->where, "address-of-label outside a function");

		(e->bits.lbl.label =
		 symtab_label_find(stab, e->bits.lbl.spel, &e->where))
			->uses++;

		/* address of label - void * */
		e->tree_type = type_ref_new_ptr(
				type_ref_new_type(type_new_primitive(type_void)),
				qual_none);

	}else{
		/* if it's an identifier, act as a read */
		fold_inc_writes_if_sym(e->lhs, stab);

		FOLD_EXPR_NO_DECAY(e->lhs, stab);

		/* can address: lvalues, arrays and functions */
		if(!expr_is_addressable(e->lhs)){
			die_at(&e->lhs->where, "can't take the address of %s (%s)",
					e->lhs->f_str(), type_ref_to_str(e->lhs->tree_type));
		}

		if(expr_kind(e->lhs, identifier)){
			decl *d = e->lhs->bits.ident.sym->decl;

			if((d->store & STORE_MASK_STORE) == store_register)
				die_at(&e->lhs->where, "can't take the address of register");
		}

		fold_disallow_bitfield(e->lhs, "taking the address of a bit-field");

		e->tree_type = type_ref_new_ptr(e->lhs->tree_type, qual_none);
	}
}

void gen_expr_addr(expr *e)
{
	if(e->bits.lbl.spel){
		out_push_lbl(e->bits.lbl.label->spel, 1); /* GNU &&lbl */

	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b"
		 * let lea_expr catch it
		 */

		lea_expr(e->lhs);
	}
}

void gen_expr_str_addr(expr *e)
{
	if(e->bits.lbl.spel){
		idt_printf("address of label \"%s\"\n", e->bits.lbl.spel);
	}else{
		idt_printf("address of expr:\n");
		gen_str_indent++;
		print_expr(e->lhs);
		gen_str_indent--;
	}
}

static void const_expr_addr(expr *e, consty *k)
{
	if(e->bits.lbl.spel){
		/*k->sym_lbl = e->bits.lbl.spel;*/
		CONST_FOLD_LEAF(k);
		k->type = CONST_ADDR;
		k->offset = 0;
		k->bits.addr.is_lbl = 1;
		k->bits.addr.bits.lbl = e->bits.lbl.label->spel;
	}else{
		const_fold(e->lhs, k);

		switch(k->type){
			case CONST_NEED_ADDR:
				/* it's a->b, a symbol, etc */
				k->type = CONST_ADDR; /* addr is const but with no value */
				break;

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

expr *expr_new_addr_lbl(char *lbl)
{
	expr *e = expr_new_wrapper(addr);
	e->bits.lbl.spel = lbl;
	return e;
}

void mutate_expr_addr(expr *e)
{
	e->f_const_fold = const_expr_addr;
}

void gen_expr_style_addr(expr *e)
{
	stylef("&(");
	gen_expr(e->lhs);
	stylef(")");
}
