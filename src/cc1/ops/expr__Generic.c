#include <assert.h>

#include "ops.h"
#include "expr__Generic.h"
#include "../type_is.h"
#include "../type_nav.h"

const char *str_expr__Generic()
{
	return "_Generic";
}

static enum lvalue_kind is_lval_generic(expr *e)
{
	return expr_is_lval(e->bits.generic.chosen->e);
}

void fold_expr__Generic(expr *e, symtable *stab)
{
	struct generic_lbl **i, *def;
	type *expr_ty;

	def = NULL;

	FOLD_EXPR(e->expr, stab);
	/* strip top level qualifiers */
	expr_ty = type_skip_all(e->expr->tree_type);

	for(i = e->bits.generic.list; i && *i; i++){
		struct generic_lbl **j, *l = *i;

		fold_expr_nodecay(l->e, stab);

		/* duplicate default checked below */
		for(j = i + 1; *j; j++){
			type *m = (*j)->t;

			/* here we check for purely equal, e.g.
			 * _Generic(..., int: x, const int: y)
			 * is valid */
			if(m && (type_cmp(m, l->t, 0) & TYPE_EQUAL_ANY)){
				fold_had_error = 1;
				warn_at_print_error(
						&(*j)->e->where,
						"duplicate type in _Generic: %s",
						type_to_str(l->t));
				continue;
			}
		}


		if(l->t){
			const char *sprob = NULL;

			fold_type(l->t, stab);

			if(!type_is_complete(l->t))
				sprob = "incomplete";
			else if(type_is_variably_modified(l->t))
				sprob = "variably-modified";
			else if(type_is_func_or_block(l->t))
				sprob = "function";

			if(sprob){
				fold_had_error = 1;
				warn_at_print_error(
						&l->e->where, "%s type '%s' in _Generic",
						sprob, type_to_str(l->t));
				continue;
			}

			switch(type_cmp(expr_ty, l->t, 0)){
				case TYPE_QUAL_ADD: /* expr=int matching const int is fine */
				case TYPE_EQUAL:
				case TYPE_EQUAL_TYPEDEF:
					if(e->bits.generic.chosen){
						assert(fold_had_error);
						continue;
					}
					e->bits.generic.chosen = l;
					break;

				default:
					break;
			}
		}else{
			if(def){
				fold_had_error = 1;
				warn_at_print_error(&def->e->where,
						"second default for _Generic");
				continue;
			}
			def = l;
		}
	}


	if(!e->bits.generic.chosen){
		if(def){
			e->bits.generic.chosen = def;
		}else{
			fold_had_error = 1;
			warn_at_print_error(&e->where,
					"no type satisfying %s",
					type_to_str(e->expr->tree_type));
			e->tree_type = type_nav_btype(cc1_type_nav, type_int);
			return;
		}
	}

	e->f_islval = is_lval_generic;

	e->tree_type = e->bits.generic.chosen->e->tree_type;

	/* direct our location to the sub-expression */
	memcpy_safe(&e->where, &e->bits.generic.chosen->e->where);
}

const out_val *gen_expr__Generic(const expr *e, out_ctx *octx)
{
	return gen_expr(e->bits.generic.chosen->e, octx);
}

void dump_expr__Generic(const expr *e, dump *ctx)
{
	struct generic_lbl **i;

	dump_desc_expr(ctx, "generic selection", e);
	dump_inc(ctx);
	dump_expr(e->expr, ctx);
	dump_dec(ctx);

	dump_inc(ctx);
	for(i = e->bits.generic.list; i && *i; i++){
		struct generic_lbl *l = *i;

		dump_inc(ctx);
		if(l->t){
			dump_printf(ctx, "%s:\n", type_to_str(l->t));
		}else{
			dump_printf(ctx, "default:\n");
		}

		dump_expr(l->e, ctx);
		dump_dec(ctx);
	}
	dump_dec(ctx);
}

static void const_expr__Generic(expr *e, consty *k)
{
	/* we're const if our chosen expr is */
	if(!e->bits.generic.chosen){
		UCC_ASSERT(fold_had_error, "_Generic const check before fold");
		CONST_FOLD_NO(k, e);
		return;
	}

	const_fold(e->bits.generic.chosen->e, k);
}

static int expr__Generic_has_sideeffects(const expr *e)
{
	struct generic_lbl *sub = e->bits.generic.chosen;
	assert(sub);
	return expr_has_sideeffects(sub->e);
}

void mutate_expr__Generic(expr *e)
{
	e->f_const_fold = const_expr__Generic;
	e->f_has_sideeffects = expr__Generic_has_sideeffects;
}

expr *expr_new__Generic(expr *test, struct generic_lbl **lbls)
{
	expr *e = expr_new_wrapper(_Generic);
	e->expr = test;
	e->bits.generic.list = lbls;
	return e;
}

const out_val *gen_expr_style__Generic(const expr *e, out_ctx *octx)
{
	struct generic_lbl **i;

	stylef("_Generic(");
	IGNORE_PRINTGEN(gen_expr(e->expr, octx));

	for(i = e->bits.generic.list; i && *i; i++){
		struct generic_lbl *l = *i;

		printf("%s: ", l->t ? type_to_str(l->t) : "default");

		IGNORE_PRINTGEN(gen_expr(l->e, octx));
	}

	return NULL;
}
