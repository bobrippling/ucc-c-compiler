#include "ops.h"
#include "expr_compound_lit.h"
#include "../out/asm.h"
#include "../decl_init.h"

const char *str_expr_compound_lit(void)
{
	return "compound-lit";
}

void fold_expr_compound_lit(expr *e, symtable *stab)
{
	decl *d = e->val.decl;

	if(e->code)
		return; /* being called from fold_gen_init_assignment_base */

	/* must be set before the recursive fold_gen_init_assignment_base */
	e->tree_type = d->ref;

	e->sym = SYMTAB_ADD(stab, d, sym_local);

	/* create the code for assignemnts */
	e->code = stmt_new_wrapper(code, stab);
	/* create assignments (even for static/global) */
	decl_init_create_assignments_for_base(d, e, e->code);

	/*
	 * update the type, for example if an array type has been completed
	 * this is done before folds, for array bounds checks
	 */
	e->tree_type = d->ref;

	if(stab->parent)
		fold_stmt(e->code); /* folds the assignments */
	else
		fold_decl_global_init(d, stab);
}

static void gen_expr_compound_lit_code(expr *e)
{
	if(!e->expr_comp_lit_cgen){
		e->expr_comp_lit_cgen = 1;

		if(e->code->symtab->parent){
			gen_stmt(e->code);
		}else{
			/* global */
			ICE("TODO: global compound initialiser");
		}
	}
}

void gen_expr_compound_lit(expr *e, symtable *stab)
{
	(void)stab;

	/* allow (int){2}, but not (struct...){...} */
	fold_disallow_st_un(e, "compound literal");

	gen_expr_compound_lit_code(e);

	out_push_sym_val(e->sym);
}

static void lea_expr_compound_lit(expr *e, symtable *stab)
{
	(void)stab;

	gen_expr_compound_lit_code(e);

	out_push_sym(e->sym);
}

void const_expr_compound_lit(expr *e, consty *k)
{
	k->type = decl_init_is_const(
			e->val.decl->init, NULL /* shouldn't be needed */)
		? CONST_WITHOUT_VAL : CONST_NO;
}

void gen_expr_str_compound_lit(expr *e, symtable *stab)
{
	decl *const d = e->val.decl;

	if(e->op)
		return;

	e->op = 1;
	{
		(void)stab;
		idt_printf("(%s){\n", decl_to_str(d));

		gen_str_indent++;
		print_decl(d,
				PDECL_NONE         |
				PDECL_INDENT       |
				PDECL_NEWLINE      |
				PDECL_SYM_OFFSET   |
				PDECL_FUNC_DESCEND |
				PDECL_PISDEF       |
				PDECL_PINIT        |
				PDECL_SIZE         |
				PDECL_ATTR);
		gen_str_indent--;

		idt_printf("}\n");
		idt_printf("init code:\n");
		print_stmt(e->code);
	}
	e->op = 0;
}

void gen_expr_style_compound_lit(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }

void mutate_expr_compound_lit(expr *e)
{
	e->f_lea = lea_expr_compound_lit;
	e->f_const_fold = const_expr_compound_lit;
}

static decl *compound_lit_decl(type_ref *t, decl_init *init)
{
	decl *d = decl_new();

	d->ref = t;
	d->init = init;
	d->is_definition = 1;

	return d;
}

void expr_compound_lit_from_cast(expr *e, decl_init *init)
{
	e->val.decl = compound_lit_decl(e->val.tref /* from cast */, init);

	expr_mutate_wrapper(e, compound_lit);
}

expr *expr_new_compound_lit(type_ref *t, decl_init *init)
{
	expr *e = expr_new_wrapper(compound_lit);
	e->val.decl = compound_lit_decl(t, init);
	return e;
}
