#include "ops.h"
#include "expr_stmt.h"

const char *str_expr_block(void)
{
	return "block";
}

void fold_expr_block(expr *e, symtable *stab)
{
	decl_desc *func;

	/* add e->block_args to symtable */

	symtab_add_args(e->code->symtab, e->block_args, "block-function");

	fold_stmt(e->code); /* symtab set to root by parse */

	UCC_ASSERT(stmt_kind(e->code, code), "!code for block");

	/*
	 * search for a return
	 * if none: void
	 * else the type of the first one we find
	 */

	if(e->decl){
		e->tree_type = e->decl;

	}else{
		stmt *r = NULL;

		stmt_walk(e->code, stmt_walk_first_return, NULL, &r);

		if(r && r->expr){
			e->tree_type = decl_copy(r->expr->tree_type);
		}else{
			e->tree_type = decl_new();
			e->tree_type->type->primitive = type_void;
		}
	}
	e->tree_type->type->store = store_static;

	/* copied the type, now make it a function */
	func = decl_desc_func_new(NULL, NULL);

	decl_desc_append(&e->tree_type->desc, func);
	decl_desc_append(&e->tree_type->desc, decl_desc_block_new(NULL, NULL));
	decl_desc_link(e->tree_type);

	func->bits.func = e->block_args;

	/* add the function to the global scope */
	e->tree_type->spel = asm_label_block(curdecl_func->spel);
	e->sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, sym_global);

	e->tree_type->func_code = e->code;

	fold_decl(e->tree_type, stab); /* funcarg folding + typedef/struct lookup, etc */
}

void gen_expr_block(expr *e, symtable *stab)
{
	(void)stab;

	/* load the function pointer */
	asm_temp(1, "mov rax, %s", e->sym->decl->spel);
	asm_temp(1, "push rax");
}

void gen_expr_str_block(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("block, type: %s, code:\n", decl_to_str(e->tree_type));
	gen_str_indent++;
	print_stmt(e->code);
	gen_str_indent--;
}

void gen_expr_style_block(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	/* TODO */
}

void mutate_expr_block(expr *e)
{
	(void)e;
}

expr *expr_new_block(decl *rt, funcargs *args, stmt *code)
{
	expr *e = expr_new_wrapper(block);
	e->block_args = args;
	e->code = code;
	e->decl = rt; /* return type if not null */
	return e;
}
