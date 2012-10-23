#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_for.h"
#include "stmt_code.h"
#include "../out/lbl.h"

const char *str_stmt_for()
{
	return "for";
}

expr *fold_for_if_init_decls(stmt *s)
{
	decl **i;
	expr *init_exp = NULL;

	for(i = s->flow->for_init_decls; *i; i++){
		decl *const d = *i;

		fold_decl(d, s->flow->for_init_symtab);

		switch(d->type->store){
			case store_auto:
			case store_default:
			case store_register:
				break;
			default:
				DIE_AT(&d->where, "%s variable in %s declaration initialisation",
						type_store_to_str(d->type->store), s->f_str());
		}

		SYMTAB_ADD(s->flow->for_init_symtab, d, sym_local);

		/* make the for-init a expr-stmt with all our inits */
		if(d->init){
			stmt *init_code;
			stmt **inits;

			init_code = stmt_new_wrapper(code, s->flow->for_init_symtab);
			fold_gen_init_assignment(d, init_code);

			init_exp = expr_new_stmt(init_code);

			for(inits = init_code->inits; inits && *inits; inits++){
				stmt *s = *inits;
				fold_stmt(s);
			}
		}
	}

	return init_exp;
}

void fold_stmt_for(stmt *s)
{
	s->lbl_break    = out_label_flow("for_start");
	s->lbl_continue = out_label_flow("for_contiune");

	if(s->flow->for_init_decls){
		expr *init_exp = fold_for_if_init_decls(s);

		UCC_ASSERT(!s->flow->for_init, "for init in C99 for-decl mode");

		s->flow->for_init = init_exp;
	}

#define FOLD_IF(x) if(x) FOLD_EXPR(x, s->flow->for_init_symtab)
	FOLD_IF(s->flow->for_init);
	FOLD_IF(s->flow->for_while);
	FOLD_IF(s->flow->for_inc);
#undef FOLD_IF

	if(s->flow->for_while){
		fold_need_expr(s->flow->for_while, "for-while", 1);

		OPT_CHECK(s->flow->for_while, "constant expression in for");
	}

	fold_stmt(s->lhs);

	/*
	 * need an extra generation for for_init,
	 * since it's generated unlike other loops (symtab_new() in parse.c)
	 */
	gen_code_decls(s->flow->for_init_symtab);

#ifdef SYMTAB_DEBUG
	fprintf(stderr, "for-code st:\n");
	PRINT_STAB(s->lhs, 1);

	fprintf(stderr, "for-init st:\n");
	print_stab(s->flow->for_init_symtab, 0, NULL);

	fprintf(stderr, "for enclosing scope st:\n");
	PRINT_STAB(s, 0);
#endif
}

void gen_stmt_for(stmt *s)
{
	char *lbl_test = out_label_flow("for_test");

	/* don't else-if, possible to have both (comma-exp for init) */
	if(s->flow->for_init){
		gen_expr(s->flow->for_init, s->flow->for_init_symtab);

		/* only pop if it's an expression (i.e. not a C99 init) */
		if(!s->flow->for_init_decls){
			out_pop();
			out_comment("unused for init");
		}
	}

	out_label(lbl_test);
	if(s->flow->for_while){
		gen_expr(s->flow->for_while, s->flow->for_init_symtab);
		out_jfalse(s->lbl_break);
	}

	gen_stmt(s->lhs);
	out_label(s->lbl_continue);
	if(s->flow->for_inc){
		gen_expr(s->flow->for_inc, s->flow->for_init_symtab);

		out_pop();
		out_comment("unused for inc");
	}

	out_push_lbl(lbl_test, 0, NULL);
	out_jmp();

	out_label(s->lbl_break);

	free(lbl_test);
}

struct walk_info
{
	stmt *escape;
	int switch_depth;
};

/* ??? change this so we set properties in fold() instead? */
static void
stmt_walk_first_break_goto_return(stmt *current, int *stop, int *descend, void *extra)
{
	struct walk_info *wi = extra;
	int found = 0;

	(void)descend;

	if(stmt_kind(current, break)){
		found = wi->switch_depth == 0;
	}else if(stmt_kind(current, return) || stmt_kind(current, goto)){
		found = 1;
	}else if(stmt_kind(current, switch)){
		wi->switch_depth++;
	}

	if(found){
		wi->escape = current;
		*stop = 1;
	}
}

static void
stmt_walk_switch_leave(stmt *current, void *extra)
{
	if(stmt_kind(current, switch)){
		struct walk_info *wi = extra;
		wi->switch_depth--;
	}
}

int fold_code_escapable(stmt *s)
{
	struct walk_info wi;

	memset(&wi, 0, sizeof wi);

	stmt_walk(s->lhs, stmt_walk_first_break_goto_return, stmt_walk_switch_leave, &wi);

	/* we only return if we find a break, goto or return statement */
	return !!wi.escape;
}

static int for_passable(stmt *s)
{
	/* if we don't have a condition, check for breaks, etc etc */
	if(s->flow->for_while)
		return 1;

	return fold_code_escapable(s);
}

void mutate_stmt_for(stmt *s)
{
	s->f_passable = for_passable;
}
