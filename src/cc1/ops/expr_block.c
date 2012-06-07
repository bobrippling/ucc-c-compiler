#include "ops.h"
#include "expr_stmt.h"

#define __unused __attribute__((unused))

const char *str_expr_block(void)
{
	return "block";
}

static void got_stmt(stmt *current, int *stop, void *extra)
{
	if(stmt_kind(current, return)){
		stmt **store = extra;
		*store = current;
		*stop = 1;
	}
}

void fold_expr_block(expr *e, symtable *stab)
{
	decl_desc *func;
	stmt *r;

	(void)stab;

	fold_stmt(e->code); /* symtab set by parse */

	UCC_ASSERT(stmt_kind(e->code, code), "!code for block");

	/*
	 * search for a return
	 * if none: void
	 * else the type of the first one we find
	 */

	stmt_walk(e->code, got_stmt, &r);

	if(r && r->expr){
		e->tree_type = decl_copy(r->expr->tree_type);
	}else{
		e->tree_type = decl_new();
		e->tree_type->type->primitive = type_void;
	}

	/* copied the type, now make it a function */
	func = decl_desc_func_new( NULL, NULL);

	decl_desc_insert(e->tree_type, decl_desc_block_new(NULL, NULL));
	decl_desc_insert(e->tree_type, func);
	decl_desc_link(e->tree_type);

	func->bits.func = e->block_args;

	/* TODO: add the function to the global scope */
}

void gen_expr_block(expr *e __unused, symtable *stab __unused)
{
	ICE("TODO");
}

void mutate_expr_block(expr *e __unused)
{
	(void)e;
}

void gen_expr_str_block(expr *e, symtable *stab)
{
	/* TODO: load the function pointer */
	ICE("TODO");
}

void gen_expr_style_block(expr *e __unused, symtable *stab __unused)
{
	ICE("TODO");
}

expr *expr_new_block(funcargs *args, stmt *code)
{
	expr *e = expr_new_wrapper(block);
	e->block_args = args;
	e->code = code;
	return e;
}
