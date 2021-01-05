#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "stmt.h"
#include "cc1.h"
#include "cc1_where.h"
#include "expr.h"
#include "fold.h"

stmt_flow *stmt_flow_new(symtable *parent)
{
	stmt_flow *t = umalloc(sizeof *t);
	t->for_init_symtab = parent;
	return t;
}

stmt *stmt_new(
		func_fold_stmt *f_fold,
		func_gen_stmt *f_gen,
		func_dump_stmt *f_dump,
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
	s->f_dump = f_dump;

	switch(cc1_backend){
		case BACKEND_ASM:
			s->f_gen = f_gen;
			break;
		case BACKEND_DUMP:
			s->f_gen = NULL;
			break;
		case BACKEND_STYLE:
			s->f_gen = f_gen_style;
			break;
		default:
			ICE("bad backend");
	}

	s->f_str  = f_str;

	init(s);

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

stmt *stmt_label_leaf(stmt *s)
{
	while(stmt_kind(s, label)
	|| stmt_kind(s, case)
	|| stmt_kind(s, case_range)
	|| stmt_kind(s, default))
	{
		s = s->lhs;
	}
	return s;
}

int stmt_is_switchlabel(const stmt *s)
{
	return stmt_kind(s, case) || stmt_kind(s, case_range) || stmt_kind(s, default);
}

int stmt_kills_below_code(stmt *s)
{
	if(!fold_passable(s, 0))
		return 1;

	if(stmt_is_switchlabel(s) || stmt_kind(s, label))
		return stmt_kills_below_code(stmt_label_leaf(s));

	return 0;
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
		stmt **i;
		for(i = base->bits.code.stmts; i && *i; i++){
			stmt_walk2(*i, enter, leave, data, stop);
			if(*stop)
				return;
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

void stmts_count(stmt *current, int *stop, int *descend, void *extra)
{
	(void)descend;
	(void)stop;
	(void)current;
	++*(int *)extra;
}

void stmt_walk(stmt *base, stmt_walk_enter enter, stmt_walk_leave leave, void *data)
{
	int stop = 0;

	stmt_walk2(base, enter, leave, data, &stop);
}

void stmt_init_blks(const stmt *ks, out_blk *con, out_blk *bbreak)
{
	stmt *s = (stmt *)ks;
	s->blk_continue = con;
	s->blk_break = bbreak;
}
