#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "decl.h"
#include "funcargs.h"
#include "cc1.h"
#include "fold.h"
#include "type_is.h"

#include "cc1_where.h"

void funcargs_free(funcargs *args, int free_decls)
{
	if(--args->retains > 0)
		return;

	if(free_decls && args){
		int i;
		for(i = 0; args->arglist[i]; i++)
			decl_free(args->arglist[i]);
	}
	free(args);
}

void funcargs_empty(funcargs *func)
{
	if(func->arglist){
		UCC_ASSERT(!func->arglist[1], "empty_args called when it shouldn't be");

		decl_free(func->arglist[0]);
		free(func->arglist);
		func->arglist = NULL;
	}
	func->args_void = 0;
}

void funcargs_empty_void(funcargs *func)
{
	/* x(void); */
	funcargs_empty(func);
	func->args_void = 1; /* (void) vs () */
}

enum funcargs_cmp funcargs_cmp(funcargs *args_to, funcargs *args_from)
{
	int count_to;
	int count_from;

	if(args_to == args_from)
		return FUNCARGS_EXACT_EQUAL;

	if(FUNCARGS_EMPTY_NOVOID(args_to)
	|| FUNCARGS_EMPTY_NOVOID(args_from))
	{
		/* a() or b() */
		return FUNCARGS_IMPLICIT_CONV;
	}

	count_to = dynarray_count(args_to->arglist);
	count_from = dynarray_count(args_from->arglist);

	/* still do prototype checks for old_proto functions */
	/*if(args_to->args_old_proto || args_from->args_old_proto)
		return FUNCARGS_IMPLICIT_CONV;*/

	if(!(args_to->variadic ? count_to <= count_from : count_to == count_from))
		return FUNCARGS_MISMATCH_COUNT;

	if(count_to){
		unsigned i;

		for(i = 0; args_to->arglist[i]; i++){
			switch(type_cmp(args_to->arglist[i]->ref, args_from->arglist[i]->ref, 0)){
				case TYPE_EQUAL:
				case TYPE_QUAL_ADD: /* f(const int) and f(int) */
				case TYPE_QUAL_SUB: /* f(int) and f(const int) */
				case TYPE_EQUAL_TYPEDEF:
					break;

				case TYPE_QUAL_POINTED_ADD:
				case TYPE_QUAL_POINTED_SUB:
				case TYPE_QUAL_NESTED_CHANGE:
				case TYPE_CONVERTIBLE_EXPLICIT:
				case TYPE_CONVERTIBLE_IMPLICIT:
				case TYPE_NOT_EQUAL:
					return FUNCARGS_MISMATCH_TYPES;
			}
		}
	}

	return FUNCARGS_EXACT_EQUAL;
}

funcargs *funcargs_new(void)
{
	funcargs *r = umalloc(sizeof *funcargs_new());
	where_cc1_current(&r->where);
	r->retains = 1;
	return r;
}

funcargs *funcargs_new_void(void)
{
	funcargs *args = funcargs_new();
	args->args_void = 1;
	return args;
}

void funcargs_ty_calc(funcargs *fa, unsigned *n_int, unsigned *n_fp)
{
	decl **di;

	*n_int = *n_fp = 0;

	for(di = fa->arglist; di && *di; di++)
		if(type_is_floating((*di)->ref))
			++*n_fp;
		else
			++*n_int;
}

int funcargs_is_old_func(funcargs *fa)
{
	/* don't treat int f(); as an old function */
	return fa->args_old_proto && fa->arglist;
}
