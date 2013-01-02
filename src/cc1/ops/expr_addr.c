#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_addr.h"
#include "../sue.h"
#include "../str.h"
#include "../out/asm.h"
#include "../out/lbl.h"
#include "../data_store.h"

const char *str_expr_addr()
{
	return "addr";
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->data_store){
		data_store_fold_type(e->data_store, &e->tree_type, stab);

	}else if(e->spel){
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
	if(e->data_store){
		out_push_lbl(e->data_store->spel, 1);

		data_store_declare(e->data_store);
		data_store_out(    e->data_store, 1);

	}else if(e->spel){
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

	if(e->data_store){
		/*asm_temp(1, "mov rax, %s", e->data_store->sym_ident);*/
		idt_printf("address of datastore %s\n", e->data_store->spel);
		gen_str_indent++;
		idt_print();
		literal_print(cc1_out, e->data_store->bits.str, e->data_store->len);
		gen_str_indent--;
		fputc('\n', cc1_out);
	}else if(e->spel){
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
	if(e->data_store){
		k->type = CONST_STRK;
		k->bits.str = e->data_store;
		k->offset = 0;
	}else if(e->spel){
		/*k->sym_lbl = e->spel;*/
		ICE("TODO");
	}else{
		const_fold(e->lhs, k);

		if(k->type == CONST_NEED_ADDR){
			/* it's a->b, a symbol, etc */
			k->type = CONST_ADDR; /* addr is const but with no value */
		}else{
			k->type = CONST_NO;
		}
	}
}

void mutate_expr_addr(expr *e)
{
	e->f_const_fold = const_expr_addr;
}

void gen_expr_style_addr(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }

void expr_mutate_addr_data(expr *e, char *s, int len)
{
	e->data_store = data_store_new_str(s, len);
}

expr *expr_new_addr_str(char *s, int len)
{
	expr *e = expr_new_wrapper(addr);
	expr_mutate_addr_data(e, s, len);
	return e;
}
