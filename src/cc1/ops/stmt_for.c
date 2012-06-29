#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_for.h"
#include "stmt_code.h"

const char *str_stmt_for()
{
	return "for";
}

#ifdef SYMTAB_DEBUG
void print_stab(symtable *st)
{
	decl **i;

	if(st->parent)
		print_stab(st->parent);

	fprintf(stderr, "\ttable %p, children %d, vars %d, parent: %p\n",
			st, dynarray_count(st->children), dynarray_count(st->decls), st->parent);

	for(i = st->decls; i && *i; i++)
		fprintf(stderr, "\t\tdecl %s\n", (*i)->spel);
}
#endif

expr *fold_for_if_init_decls(stmt *s)
{
	decl **i;
	expr *init_exp = NULL;

	for(i = s->flow->for_init_decls; *i; i++){
		decl *const d = *i;

		fold_decl(d, s->flow->for_init_symtab);

		SYMTAB_ADD(s->flow->for_init_symtab, d, sym_local);

		/* make the for-init a comma-exp with all our inits */
		if(d->init){
			expr *dinit = expr_new_decl_init(d);

			if(init_exp){
				expr *comma = expr_new_comma(); /* change this to an &&-op for if(char *x = .., *y = ..) anding */
				comma->lhs = init_exp;
				comma->rhs = dinit;
				init_exp = comma;
			}else{
				init_exp = dinit;
			}
		}
	}

	return init_exp;
}

void fold_stmt_for(stmt *s)
{
	s->lbl_break    = asm_label_flow("for_start");
	s->lbl_continue = asm_label_flow("for_contiune");

	if(s->flow->for_init_decls){
		expr *init_exp = fold_for_if_init_decls(s);

		UCC_ASSERT(!s->flow->for_init, "for init in c99 for-decl mode");

		s->flow->for_init = init_exp;
	}

#define FOLD_IF(x) if(x) fold_expr(x, s->flow->for_init_symtab)
	FOLD_IF(s->flow->for_init);
	FOLD_IF(s->flow->for_while);
	FOLD_IF(s->flow->for_inc);
#undef FOLD_IF

	if(s->flow->for_while){
		fold_test_expr(s->flow->for_while, "for-while");

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
	print_stab(s->lhs->symtab);

	fprintf(stderr, "for-init st:\n");
	print_stab(s->flow->for_init_symtab);

	fprintf(stderr, "for enclosing scope st:\n");
	print_stab(s->symtab);
#endif
}

void gen_stmt_for(stmt *s)
{
	char *lbl_test = asm_label_flow("for_test");

	/* don't else-if, possible to have both (comma-exp for init) */
	if(s->flow->for_init){
		gen_expr(s->flow->for_init, s->flow->for_init_symtab);
		asm_temp(1, "pop rax ; unused for init");
	}

	asm_label(lbl_test);
	if(s->flow->for_while){
		gen_expr(s->flow->for_while, s->flow->for_init_symtab);

		asm_temp(1, "pop rax");
		asm_temp(1, "test rax, rax");
		asm_temp(1, "jz %s", s->lbl_break);
	}

	gen_stmt(s->lhs);
	asm_label(s->lbl_continue);
	if(s->flow->for_inc){
		gen_expr(s->flow->for_inc, s->flow->for_init_symtab);
		asm_temp(1, "pop rax ; unused for inc");
	}

	asm_temp(1, "jmp %s", lbl_test);

	asm_label(s->lbl_break);

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
