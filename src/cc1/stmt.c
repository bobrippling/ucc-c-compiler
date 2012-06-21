#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "stmt.h"

stmt_flow *stmt_flow_new(symtable *parent)
{
	stmt_flow *t = umalloc(sizeof *t);
	t->for_init_symtab = parent;
	return t;
}

void stmt_mutate(stmt *s,
		func_fold_stmt *f_fold, func_gen_stmt *f_gen, func_str_stmt *f_str,
		func_mutate_stmt *f_mutate)
{
	s->f_fold = f_fold;
	s->f_gen  = f_gen;
	s->f_str  = f_str;

	s->kills_below_code =
		   stmt_kind(s, break)
		|| stmt_kind(s, return)
		|| stmt_kind(s, goto)
		|| stmt_kind(s, continue);

	f_mutate(s);
}

stmt *stmt_new(func_fold_stmt *f_fold, func_gen_stmt *f_gen, func_str_stmt *f_str,
		func_mutate_stmt *f_mutate, symtable *stab)
{
	stmt *s = umalloc(sizeof *s);
	where_new(&s->where);

	UCC_ASSERT(stab, "no symtable for statement");
	s->symtab = stab;

	stmt_mutate(s, f_fold, f_gen, f_str, f_mutate);

	return s;
}

static void stmt_walk2(stmt *base, void (*f)(stmt *current, int *stop, void *), void *data, int *stop)
{
	f(base, stop, data);

	if(*stop)
		return;

#define WALK_IF(sub) if(sub){ f(sub, stop, data); if(*stop) return; }

	WALK_IF(base->lhs);
	WALK_IF(base->rhs);

	if(base->codes){
		int i;
		for(i = 0; base->codes[i]; i++){
			stmt_walk2(base->codes[i], f, data, stop);
			if(*stop)
				break;
		}
	}
}

void stmt_walk_first_return(stmt *current, int *stop, void *extra)
{
	if(stmt_kind(current, return)){
		stmt **store = extra;
		*store = current;
		*stop = 1;
	}
}

void stmt_walk(stmt *base, void (*f)(stmt *current, int *stop, void *), void *data)
{
	int stop = 0;

	stmt_walk2(base, f, data, &stop);
}
