#include "ops.h"
#include "expr__Generic.h"

const char *str_expr__Generic()
{
	return "_Generic";
}

void fold_expr__Generic(expr *e, symtable *stab)
{
	struct generic_lbl **i, *def;

	(void)stab;

	def = NULL;

	FOLD_EXPR(e->expr, stab);

	for(i = e->generics; i && *i; i++){
		const int flags = DECL_CMP_EXACT_MATCH;
		struct generic_lbl **j, *l = *i;

		FOLD_EXPR(l->e, stab);

		for(j = i + 1; *j; j++){
			decl *m = (*j)->d;

			/* duplicate default checked below */
			if(m && type_ref_equal(m, l->d, flags))
				DIE_AT(&m->where, "duplicate type in _Generic: %s", decl_to_str(l->d));
		}


		if(l->d){
			fold_decl(l->d, stab);

			if(type_ref_equal(e->expr->tree_type, l->d, flags)){
				UCC_ASSERT(!e->generic_chosen, "already chosen expr for _Generic");
				e->generic_chosen = l;
			}
		}else{
			if(def)
				DIE_AT(&def->e->where, "second default for _Generic");
			def = l;
		}
	}


	if(!e->generic_chosen){
		if(def)
			e->generic_chosen = def;
		else
			DIE_AT(&e->where, "no type satisfying %s", decl_to_str(e->expr->tree_type));
	}

	e->tree_type = decl_copy(e->generic_chosen->e->tree_type);
}

void gen_expr__Generic(expr *e, symtable *stab)
{
	gen_expr(e->generic_chosen->e, stab);
}

void gen_expr_str__Generic(expr *e, symtable *stab)
{
	struct generic_lbl **i;

	(void)stab;

	idt_printf("_Generic expr:\n");
	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;

	idt_printf("_Generic choices:\n");
	gen_str_indent++;
	for(i = e->generics; i && *i; i++){
		struct generic_lbl *l = *i;

		if(e->generic_chosen == l)
			idt_printf("-- Chosen --\n");

		if(l->d){
			idt_printf("type:\n");
			gen_str_indent++;
			print_decl(l->d, PDECL_INDENT | PDECL_NEWLINE);
			gen_str_indent--;
		}else{
			idt_printf("default:\n");
		}
		idt_printf("expr:\n");
		gen_str_indent++;
		print_expr(l->e);
		gen_str_indent--;
	}
	gen_str_indent--;
}

void const_expr__Generic(expr *e, intval *piv, enum constyness *pconst_type)
{
	/* we're const if our chosen expr is */
	UCC_ASSERT(e->generic_chosen, "_Generic const check before fold");

	const_fold(e->generic_chosen->e, piv, pconst_type);
}

void mutate_expr__Generic(expr *e)
{
	e->f_const_fold = const_expr__Generic;
}

expr *expr_new__Generic(expr *test, struct generic_lbl **lbls)
{
	expr *e = expr_new_wrapper(_Generic);
	e->expr = test;
	e->generics = lbls;
	return e;
}

void gen_expr_style__Generic(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
