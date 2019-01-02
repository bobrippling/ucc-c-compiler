#include <string.h>
#include <assert.h>

#include "ops.h"
#include "expr_compound_lit.h"
#include "../out/asm.h"
#include "../out/lbl.h"
#include "../decl_init.h"
#include "../type_is.h"

#define COMP_LIT_INITIALISED(e) (e)->bits.complit.decl->bits.var.init.expr

const char *str_expr_compound_lit(void)
{
	return "compound-lit";
}

void fold_expr_compound_lit(expr *e, symtable *stab)
{
	decl *d = e->bits.complit.decl;
	int static_ctx = e->bits.complit.static_ctx; /* global or static */

	if(cc1_std < STD_C99)
		cc1_warn_at(&e->where, c89_compound_literal,
				"compound literals are a C99 feature");

	/* if(!stab->parent) assert(static_ctx);
	 *
	 * except things like sizeof() just pass 0 for static_ctx,
	 * as it doesn't matter, we're not code-gen'd
	 */
	if(!stab->parent)
		static_ctx = 1;

	if(COMP_LIT_INITIALISED(e))
		return; /* being called from fold_gen_init_assignment_base */

	/* must be set before the recursive fold_gen_init_assignment_base */
	e->tree_type = d->ref;

	if(static_ctx){
		assert(!d->spel_asm);
		d->spel_asm = out_label_data_store(STORE_COMP_LIT);
		d->store = store_static;
	}

	e->bits.complit.sym = sym_new_and_prepend_decl(
			stab, d, static_ctx ? sym_global : sym_local);

	/* fold the initialiser */
	UCC_ASSERT(d->bits.var.init.dinit, "no init for comp.literal");

	decl_init_brace_up_fold(d, stab);

	/*
	 * update the type, for example if an array type has been completed
	 * this is done before folds, for array bounds checks
	 */
	e->tree_type = d->ref;

	if(!static_ctx){
		/* create the code for assignemnts
		 *
		 * - we must create a nested scope,
		 *   otherwise any other decls in stab's scope will
		 *   be generated twice - once for the scope we're nested in (stab),
		 *   and again on our call to gen_stmt() in our gen function
		 */
		decl_init_create_assignments_base_and_fold(d, e, stab);
	}else{
		fold_decl_global_init(d, stab);
	}
}

static void gen_expr_compound_lit_code(const expr *e, out_ctx *octx)
{
	if(!e->expr_comp_lit_cgen){
		expr *initexp = e->bits.complit.decl->bits.var.init.expr;

		/* prevent the sub gen_expr() call from coming back in here
		 * when references to the compound literal symbol are generated.
		 *
		 * this is undone afterwards to allow re-entry per non-recursive
		 * gen_expr() call, for example, function inlining means this
		 * expression may be generated more than once
		 */
		GEN_CONST_CAST(expr *, e)->expr_comp_lit_cgen = 1;

		if(initexp)
			out_val_consume(octx, gen_expr(initexp, octx));

		GEN_CONST_CAST(expr *, e)->expr_comp_lit_cgen = 0;
	}
}

const out_val *gen_expr_compound_lit(const expr *e, out_ctx *octx)
{
	gen_asm_emit_type(octx, e->tree_type);
	gen_expr_compound_lit_code(e, octx);

	return out_new_sym(octx, e->bits.complit.sym);
}

static void const_expr_compound_lit(expr *e, consty *k)
{
	decl *d = e->bits.complit.decl;
	expr *nonstd = NULL;

	if(decl_init_is_const(d->bits.var.init.dinit, NULL, d->ref, &nonstd, NULL)
	&& decl_store_duration_is_static(d))
	{
		CONST_FOLD_LEAF(k);
		k->type = CONST_ADDR_OR_NEED(d);
		k->bits.addr.is_lbl = 1;
		k->bits.addr.bits.lbl = decl_asm_spel(d);
		k->offset = 0;
		if(!k->nonstandard_const)
			k->nonstandard_const = nonstd;
	}else{
		CONST_FOLD_NO(k, e);
	}
}

void dump_expr_compound_lit(const expr *e, dump *ctx)
{
	decl *const d = e->bits.complit.decl;

	if(e->expr_comp_lit_cgen)
		return;

	GEN_CONST_CAST(expr *, e)->expr_comp_lit_cgen = 1;

	dump_desc_expr(ctx, "compound literal", e);

	dump_inc(ctx);
	dump_init(ctx, d->bits.var.init.dinit);
	dump_desc_expr(ctx, "compound literal generated init", e);
	dump_inc(ctx);
	dump_expr(e->bits.complit.decl->bits.var.init.expr, ctx);
	dump_dec(ctx);
	dump_dec(ctx);

	GEN_CONST_CAST(expr *, e)->expr_comp_lit_cgen = 0;
}

const out_val *gen_expr_style_compound_lit(const expr *e, out_ctx *octx)
{
	stylef("(%s)", type_to_str(e->bits.complit.decl->ref));
	gen_style_dinit(expr_comp_lit_init(e));
	UNUSED_OCTX();
}

static int expr_compound_lit_has_sideeffects(const expr *e)
{
	return decl_init_has_sideeffects(expr_comp_lit_init(e));
}

void mutate_expr_compound_lit(expr *e)
{
	/* unconditionally an lvalue */
	e->f_islval = expr_is_lval_always;
	e->f_const_fold = const_expr_compound_lit;
	e->f_has_sideeffects = expr_compound_lit_has_sideeffects;
}

static decl *compound_lit_decl(type *t, decl_init *init)
{
	decl *d = decl_new();

	d->ref = t;
	d->bits.var.init.dinit = init;

	return d;
}

expr *expr_new_compound_lit(type *t, decl_init *init, int static_ctx)
{
	expr *e = expr_new_wrapper(compound_lit);
	e->bits.complit.decl = compound_lit_decl(t, init);
	e->bits.complit.static_ctx = static_ctx;
	return e;
}
