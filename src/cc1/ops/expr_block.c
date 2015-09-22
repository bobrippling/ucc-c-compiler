#include <string.h>

#include "ops.h"
#include "expr_block.h"
#include "../out/lbl.h"
#include "../../util/dynarray.h"
#include "../funcargs.h"
#include "../type_nav.h"

const char *str_expr_block(void)
{
	return "block";
}

void expr_block_set_ty(decl *db, type *retty, symtable *scope)
{
	expr *e = db->block_expr;

	db->ref = type_func_of(
				retty,
				e->bits.block.args,
				symtab_new(scope, &e->where));
}

void expr_block_got_params(
		expr *e,
		symtable *symtab, funcargs *args)
{
	decl *df = decl_new();

	df->spel = out_label_block("globl");
	df->block_expr = e;

	fold_funcargs(args, symtab, NULL);

	symtab->in_func = df;

	/* add a global symbol for the block */
	e->bits.block.sym = sym_new_and_prepend_decl(
			symtab_root(symtab), df, sym_global);
}

void expr_block_got_code(expr *e, stmt *code)
{
	e->code = code;
}

/*
 * TODO:
 * search e->code for expr_identifier,
 * and it it's not in e->code->symtab or lower,
 * add it as a block argument
 *
 * int i;
 * ^{
 *   i = 5;
 * }();
 */

void fold_expr_block(expr *e, symtable *scope_stab)
{
	/* prevent access to nested vars */
	symtable *const arg_symtab = e->code->symtab->parent;
	symtable *const sym_root = symtab_root(arg_symtab);
	decl *const df = arg_symtab->in_func;

	/* fold block code (needs to be done before return-type of block) */
	UCC_ASSERT(stmt_kind(e->code, code), "!code for block");

	df->bits.func.code = e->code;

	if(e->bits.block.retty){
		expr_block_set_ty(df, e->bits.block.retty, scope_stab);
	}else{
		/* df->ref is NULL until we find a return statement,
		 * which sets df->ref for us */
	}

	/* arguments and code, then we have the type for the decl */
	fold_funcargs(e->bits.block.args, arg_symtab, NULL);
	fold_func_code(e->code, &e->where, "block", arg_symtab);

	/* if we didn't hit any returns, we're a void block */
	if(!df->ref)
		expr_block_set_ty(df, type_nav_btype(cc1_type_nav, type_void), scope_stab);

	/* said decl: */
	fold_decl(df, sym_root);

	/* block pointer to the function */
	e->tree_type = type_block_of(df->ref);

	fold_func_is_passable(df, type_called(df->ref, NULL), 1);
}

static void const_expr_block(expr *e, consty *k)
{
	/* all blocks are const currently, since they're not capturing,
	 * just static function references */
	CONST_FOLD_LEAF(k);

	k->type = CONST_ADDR;
	k->bits.addr.is_lbl = 1;
	k->bits.addr.bits.lbl = decl_asm_spel(e->bits.block.sym->decl);
}

const out_val *gen_expr_block(const expr *e, out_ctx *octx)
{
	return out_new_sym(octx, e->bits.block.sym);
}

irval *gen_ir_expr_block(const expr *e, irctx *ctx)
{
	return irval_from_sym(e->bits.block.sym);
}

void dump_expr_block(const expr *e, dump *ctx)
{
	dump_desc_expr(ctx, "block", e);
	dump_inc(ctx);
	dump_stmt(e->code, ctx);
	dump_dec(ctx);
}

const out_val *gen_expr_style_block(const expr *e, out_ctx *octx)
{
	stylef("^%s", type_to_str(e->tree_type));
	gen_stmt(e->code, octx);
	return NULL;
}

void mutate_expr_block(expr *e)
{
	e->f_const_fold = const_expr_block;
}

expr *expr_new_block(type *rt, funcargs *args)
{
	expr *e = expr_new_wrapper(block);
	e->bits.block.args = args;
	e->bits.block.retty = rt; /* return type if not null */
	return e;
}
