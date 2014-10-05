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
		d->spel_asm = out_label_data_store(STORE_COMP_LIT);
		d->store = store_static;
	}

	e->bits.complit.sym = sym_new_and_prepend_decl(
			stab, d, static_ctx ? sym_global : sym_local);

	/* fold the initialiser */
	UCC_ASSERT(d->bits.var.init.dinit, "no init for comp.literal");

	decl_init_brace_up_fold(d, stab, /*initial struct copy:*/0);

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

static void gen_expr_compound_lit_code(expr *e, out_ctx *octx)
{
	if(!e->expr_comp_lit_cgen){
		expr *initexp = e->bits.complit.decl->bits.var.init.expr;

		e->expr_comp_lit_cgen = 1;

		if(initexp)
			out_val_consume(octx, gen_expr(initexp, octx));
	}
}

const out_val *gen_expr_compound_lit(expr *e, out_ctx *octx)
{
	/* allow (int){2}, but not (struct...){...} */
	fold_check_expr(e, FOLD_CHK_NO_ST_UN, "compound literal");

	gen_expr_compound_lit_code(e, octx);

	return out_new_sym_val(octx, e->bits.complit.sym);
}

static const out_val *lea_expr_compound_lit(expr *e, out_ctx *octx)
{
	gen_expr_compound_lit_code(e, octx);

	return out_new_sym(octx, e->bits.complit.sym);
}

static void const_expr_compound_lit(expr *e, consty *k)
{
	decl *d = e->bits.complit.decl;
	expr *nonstd = NULL;

	if(decl_init_is_const(d->bits.var.init.dinit, NULL, &nonstd)){
		CONST_FOLD_LEAF(k);
		k->type = CONST_ADDR_OR_NEED(d);
		k->bits.addr.is_lbl = 1;
		k->bits.addr.bits.lbl = decl_asm_spel(d);
		k->offset = 0;
		k->nonstandard_const = nonstd;
	}else{
		k->type = CONST_NO;
	}
}

const out_val *gen_expr_str_compound_lit(expr *e, out_ctx *octx)
{
	decl *const d = e->bits.complit.decl;

	if(e->bits.op.op)
		return NULL;

	e->bits.op.op = 1;
	{
		idt_printf("(%s){\n", decl_to_str(d));

		gen_str_indent++;
		print_decl(d,
				PDECL_NONE         |
				PDECL_INDENT       |
				PDECL_NEWLINE      |
				PDECL_SYM_OFFSET   |
				PDECL_FUNC_DESCEND |
				PDECL_PINIT        |
				PDECL_SIZE         |
				PDECL_ATTR);
		gen_str_indent--;

		idt_printf("}\n");
		if(e->code){
			idt_printf("init code:\n");
			print_stmt(e->code);
		}
	}
	e->bits.op.op = 0;

	UNUSED_OCTX();
}

const out_val *gen_expr_style_compound_lit(expr *e, out_ctx *octx)
{
	stylef("(%s)", type_to_str(e->bits.complit.decl->ref));
	gen_style_dinit(e->bits.complit.decl->bits.var.init.dinit);
	UNUSED_OCTX();
}

void mutate_expr_compound_lit(expr *e)
{
	/* unconditionally an lvalue */
	e->f_lea = lea_expr_compound_lit;
	e->f_islval = expr_is_lval_always;
	e->f_const_fold = const_expr_compound_lit;
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
