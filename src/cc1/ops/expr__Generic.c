#include "ops.h"
#include "expr__Generic.h"

const char *str_expr__Generic()
{
	return "_Generic";
}

void fold_expr__Generic(expr *e, symtable *stab)
{
	struct generic_lbl **i, *def;

	def = NULL;

	FOLD_EXPR(e->expr, stab);

	for(i = e->bits.generic.list; i && *i; i++){
		struct generic_lbl **j, *l = *i;

		FOLD_EXPR(l->e, stab);

		for(j = i + 1; *j; j++){
			type_ref *m = (*j)->t;

			/* duplicate default checked below */
			if(m && type_ref_cmp(m, l->t, 0) == TYPE_EQUAL)
				die_at(&m->where, "duplicate type in _Generic: %s", type_ref_to_str(l->t));
		}


		if(l->t){
			fold_type_ref(l->t, NULL, stab);

			if(type_ref_cmp(e->expr->tree_type, l->t, 0) == TYPE_EQUAL){
				UCC_ASSERT(!e->bits.generic.chosen, "already chosen expr for _Generic");
				e->bits.generic.chosen = l;
			}
		}else{
			if(def)
				die_at(&def->e->where, "second default for _Generic");
			def = l;
		}
	}


	if(!e->bits.generic.chosen){
		if(def)
			e->bits.generic.chosen = def;
		else
			die_at(&e->where, "no type satisfying %s", type_ref_to_str(e->expr->tree_type));
	}

	e->tree_type = e->bits.generic.chosen->e->tree_type;
}

basic_blk *gen_expr__Generic(expr *e, basic_blk *bb)
{
	bb = gen_expr(e->bits.generic.chosen->e, bb);
	return bb;
}

basic_blk *gen_expr_str__Generic(expr *e, basic_blk *bb)
{
	struct generic_lbl **i;

	idt_printf("_Generic expr:\n");
	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;

	idt_printf("_Generic choices:\n");
	gen_str_indent++;
	for(i = e->bits.generic.list; i && *i; i++){
		struct generic_lbl *l = *i;

		if(e->bits.generic.chosen == l)
			idt_printf("[Chosen]\n");

		if(l->t){
			idt_printf("type: ");
			gen_str_indent++;
			print_type_ref(l->t, NULL);
			gen_str_indent--;
			fprintf(gen_file(), "\n");
		}else{
			idt_printf("default:\n");
		}
		idt_printf("expr:\n");
		gen_str_indent++;
		print_expr(l->e);
		gen_str_indent--;
	}
	gen_str_indent--;

	return bb;
}

static void const_expr__Generic(expr *e, consty *k)
{
	/* we're const if our chosen expr is */
	UCC_ASSERT(e->bits.generic.chosen, "_Generic const check before fold");

	const_fold(e->bits.generic.chosen->e, k);
}

void mutate_expr__Generic(expr *e)
{
	e->f_const_fold = const_expr__Generic;
}

expr *expr_new__Generic(expr *test, struct generic_lbl **lbls)
{
	expr *e = expr_new_wrapper(_Generic);
	e->expr = test;
	e->bits.generic.list = lbls;
	return e;
}

basic_blk *gen_expr_style__Generic(expr *e, basic_blk *bb)
{
	struct generic_lbl **i;

	stylef("_Generic(");
	bb = gen_expr(e->expr, bb);

	for(i = e->bits.generic.list; i && *i; i++){
		struct generic_lbl *l = *i;

		idt_printf("%s: ",
				l->t ? type_ref_to_str(l->t) : "default");

		bb = gen_expr(l->e, bb);
	}

	return bb;
}
