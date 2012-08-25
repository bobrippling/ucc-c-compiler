#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "../sue.h"
#include "../str.h"
#include "../out/asm.h"
#include "../out/lbl.h"

const char *str_expr_addr()
{
	return "addr";
}

void fold_expr_addr(expr *e, symtable *stab)
{
#define TT e->tree_type

	if(e->array_store){
		sym *array_sym;

		/*UCC_ASSERT(!e->sym, "symbol found when looking for array store");*/
		UCC_ASSERT(!e->lhs, "expression found in array store address-of");

		/* static const char [] */
		TT = decl_new();

		TT->desc = decl_desc_array_new(e->tree_type, NULL);
		TT->desc->bits.array_size = expr_new_val(e->array_store->len);

		TT->type->store = store_static;
		TT->type->qual  = qual_const;

		e->spel = e->array_store->label = out_label_array(e->array_store->type == array_str);

		decl_set_spel(e->tree_type, e->spel);

		array_sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, stab->parent ? sym_local : sym_global);

		array_sym->decl->arrayinit = e->array_store;
		array_sym->decl->internal = 1;


		switch(e->array_store->type){
			case array_str:
				e->tree_type->type->primitive = type_char;
				TT->type->primitive = type_char;
				break;

			case array_exprs:
			{
				expr **inits;
				int i;

				TT->type->primitive = type_int;

				inits = e->array_store->data.exprs;

				for(i = 0; inits[i]; i++){
					enum constyness const_type;
					intval val;

					fold_expr(inits[i], stab);
					const_fold(inits[i], &val, &const_type);

					if(const_type == CONST_NO)
						DIE_AT(&inits[i]->where, "array init not constant (%s)", inits[i]->f_str());
				}
			}
		}

	}else if(e->spel){
		char *save;

		/* address of label - void * */
		e->tree_type = decl_ptr_depth_inc(decl_new_void());

		save = e->spel;
		e->spel = out_label_goto(e->spel);
		free(save);

	}else{
		fold_inc_writes_if_sym(e->lhs, stab);

		fold_expr(e->lhs, stab);

		/* lvalues are identifier, struct-exp or deref */
		if(!expr_is_lvalue(e->lhs, LVAL_ALLOW_FUNC | LVAL_ALLOW_ARRAY))
			DIE_AT(&e->lhs->where, "can't take the address of %s (%s)",
					e->lhs->f_str(), decl_to_str(e->lhs->tree_type));


		if(e->lhs->tree_type->type->store == store_register)
			DIE_AT(&e->lhs->where, "can't take the address of register variable %s", e->lhs->spel);

		e->tree_type = decl_ptr_depth_inc(decl_copy(e->lhs->sym ? e->lhs->sym->decl : e->lhs->tree_type));
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	if(e->array_store){
		/*decl *d = e->array_store->data.exprs[0];*/
		out_push_lbl(e->array_store->label, 1, e->tree_type);

	}else if(e->spel){
		out_push_lbl(e->spel, 1, NULL); /* GNU &&lbl */

	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b" */
		if(expr_kind(e->lhs, struct))
			UCC_ASSERT(!e->lhs->expr_is_st_dot, "not &x->y");
		else
			UCC_ASSERT(expr_kind(e->lhs, identifier) || expr_kind(e->lhs, deref),
					"invalid addr");

		lea_expr(e->lhs, stab);
	}
}

void gen_expr_addr_1(expr *e, FILE *f)
{
	/* TODO: merge this code with gen_addr / walk_expr with expr_addr */
	if(e->array_store){
		/* address of an array store */
		asm_declare_out(f, NULL, "%s", e->array_store->label);

	}else if(e->spel){
		asm_declare_out(f, NULL, "%s", e->spel);

	}else if(expr_kind(e->lhs, identifier)){
		asm_declare_out(f, NULL, "%s", e->lhs->spel);

	}else{
		intval iv;
		enum constyness type;

		const_fold(e->lhs, &iv, &type);

		UCC_ASSERT(type == CONST_WITH_VAL, "invalid constant expression");

		asm_declare_out(f, NULL, "%ld", iv.val);
	}
}

void gen_expr_str_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		if(e->array_store->type == array_str){
			idt_printf("label: %s, \"", e->array_store->label);
			literal_print(cc1_out, e->array_store->data.str, e->array_store->len);
			fprintf(cc1_out, "\" (length=%d)\n", e->array_store->len);
		}else{
			int i;
			idt_printf("array: %s:\n", e->array_store->label);
			gen_str_indent++;
			for(i = 0; e->array_store->data.exprs[i]; i++){
				idt_printf("array[%d]:\n", i);
				gen_str_indent++;
				print_expr(e->array_store->data.exprs[i]);
				gen_str_indent--;
			}
			gen_str_indent--;
		}
	}else if(e->spel){
		idt_printf("address of label \"%s\"\n", e->spel);
	}else{
		idt_printf("address of expr:\n");
		gen_str_indent++;
		print_expr(e->lhs);
		gen_str_indent--;
	}
}

void const_expr_addr(expr *e, intval *iv, enum constyness *ptype)
{
	(void)e;
	(void)iv;
	*ptype = CONST_WITHOUT_VAL;
}

void mutate_expr_addr(expr *e)
{
	e->f_gen_1 = gen_expr_addr_1;
	e->f_const_fold = const_expr_addr;
}

void gen_expr_style_addr(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
