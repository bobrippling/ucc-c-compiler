#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_addr.h"
#include "../out/lbl.h"

const char *str_expr_addr()
{
	return "addr";
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->spel){
		char *save;

		/* address of label - void * */
		e->tree_type = type_ref_new_ptr(
				type_ref_new_type(type_new_primitive(type_void)),
				qual_none);

		save = e->spel;
		e->spel = out_label_goto(e->spel);
		free(save);

	}else{
		fold_inc_writes_if_sym(e->lhs, stab);

		FOLD_EXPR_NO_DECAY(e->lhs, stab);

		/* can address: lvalues, arrays and functions */
		if(!expr_is_lvalue(e->lhs)
		&& !type_ref_is(e->lhs->tree_type, type_ref_array)
		&& !type_ref_is(e->lhs->tree_type, type_ref_func))
		{
			DIE_AT(&e->lhs->where, "can't take the address of %s (%s)",
					e->lhs->f_str(), type_ref_to_str(e->lhs->tree_type));
		}

#ifdef FIELD_WIDTH_TODO
		if(e->lhs->tree_type->field_width)
			DIE_AT(&e->lhs->where, "taking the address of a bit-field");
#endif

		if(expr_kind(e->lhs, identifier) && e->lhs->sym->decl->store == store_register)
			DIE_AT(&e->lhs->where, "can't take the address of register");

		e->tree_type = type_ref_new_ptr(e->lhs->tree_type, qual_none);
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	if(e->spel){
		out_push_lbl(e->spel, 1); /* GNU &&lbl */

	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b"
		 * let lea_expr catch it
		 */

		lea_expr(e->lhs, stab);
	}
}

void gen_expr_str_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->spel){
		idt_printf("address of label \"%s\"\n", e->spel);
	}else{
		idt_printf("address of expr:\n");
		gen_str_indent++;
		print_expr(e->lhs);
		gen_str_indent--;
	}
}

void const_expr_addr(expr *e, consty *k)
{
	if(e->spel){
		/*k->sym_lbl = e->spel;*/
		k->type = CONST_ADDR;
		k->offset = 0;
		k->bits.addr.is_lbl = 1;
		k->bits.addr.bits.lbl = e->spel;
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

void mutate_expr_addr(expr *e)
{
	e->f_const_fold = const_expr_addr;
}

void gen_expr_style_addr(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
