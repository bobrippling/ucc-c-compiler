#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "stmt.h"
#include "cc1.h"
#include "cc1_where.h"
#include "expr.h"

stmt_flow *stmt_flow_new(symtable *parent)
{
	stmt_flow *t = umalloc(sizeof *t);
	t->for_init_symtab = parent;
	return t;
}

stmt *stmt_new(
		func_fold_stmt *f_fold,
		func_gen_stmt *f_gen,
		func_gen_stmt *f_gen_style,
		func_str_stmt *f_str,
		void (*init)(stmt *),
		symtable *stab)
{
	stmt *s = umalloc(sizeof *s);
	where_cc1_current(&s->where);

	UCC_ASSERT(stab, "no symtable for statement");
	s->symtab = stab;

	s->f_fold = f_fold;

	switch(cc1_backend){
		case BACKEND_ASM:
			s->f_gen = f_gen;
			break;
		case BACKEND_PRINT:
		case BACKEND_STYLE:
			s->f_gen = f_gen_style;
			break;
		default:
			ICE("bad backend");
	}

	s->f_str  = f_str;

	init(s);

	s->kills_below_code =
		   stmt_kind(s, break)
		|| stmt_kind(s, return)
		|| stmt_kind(s, goto)
		|| stmt_kind(s, continue);

	return s;
}

stmt *expr_to_stmt(expr *e, symtable *scope)
{
	stmt *t = stmt_new_wrapper(expr, scope);
	t->expr = e;
	return stmt_set_where(t, &e->where);
}

stmt *stmt_set_where(stmt *s, where const *w)
{
	memcpy_safe(&s->where, w);
	return s;
}

static void stmt_walk2(stmt *base, stmt_walk_enter enter, stmt_walk_leave leave, void *data, int *stop)
{
	int descend = 1;

	enter(base, stop, &descend, data);

	if(*stop)
		return;

#define WALK_IF(sub) if(sub){ stmt_walk2(sub, enter, leave, data, stop); if(*stop) return; }

	if(!descend)
		goto fin;

	WALK_IF(base->lhs);
	WALK_IF(base->rhs);

	if(stmt_kind(base, code) && base->bits.code.stmts){
		int i;
		for(i = 0; base->bits.code.stmts[i]; i++){
			stmt_walk2(base->bits.code.stmts[i], enter, leave, data, stop);
			if(*stop)
				break;
		}
	}

fin:
	if(leave)
		leave(base, data);
}

void stmt_walk_first_return(stmt *current, int *stop, int *descend, void *extra)
{
	(void)descend;

	if(stmt_kind(current, return)){
		stmt **store = extra;
		*store = current;
		*stop = 1;
	}
}

void stmt_walk(stmt *base, stmt_walk_enter enter, stmt_walk_leave leave, void *data)
{
	int stop = 0;

	stmt_walk2(base, enter, leave, data, &stop);
}
