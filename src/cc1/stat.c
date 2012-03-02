#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "stat.h"

stat_flow *stat_flow_new()
{
	stat_flow *t = umalloc(sizeof *t);
	return t;
}

void stat_mutate(stat *s, func_fold_stat *f_fold, func_gen_stat *f_gen, func_str_stat *f_str)
{
	s->f_fold = f_fold;
	s->f_gen  = f_gen;
	s->f_str  = f_str;
}

stat *stat_new(func_fold_stat *f_fold, func_gen_stat *f_gen, func_str_stat *f_str, symtable *stab)
{
	stat *s = umalloc(sizeof *s);
	where_new(&s->where);

	stat_mutate(s, f_fold, f_gen, f_str);

	UCC_ASSERT(stab, "no symtable for statement");
	s->symtab = stab;

	return s;
}

