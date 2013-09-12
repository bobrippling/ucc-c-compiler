#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "decl.h"
#include "funcargs.h"
#include "cc1.h"
#include "fold.h"

void funcargs_free(funcargs *args, int free_decls, int free_refs)
{
	if(free_decls && args){
		int i;
		for(i = 0; args->arglist[i]; i++)
			decl_free(args->arglist[i], free_refs);
	}
	free(args);
}

void funcargs_empty(funcargs *func)
{
	if(func->arglist){
		UCC_ASSERT(!func->arglist[1], "empty_args called when it shouldn't be");

		decl_free(func->arglist[0], 1);
		free(func->arglist);
		func->arglist = NULL;
	}
	func->args_void = 0;
}

enum funcargs_cmp funcargs_equal(
		funcargs *args_to, funcargs *args_from,
		int exact, const char *fspel)
{
	const int count_to = dynarray_count(args_to->arglist);
	const int count_from = dynarray_count(args_from->arglist);

	if((count_to   == 0 && !args_to->args_void)
	|| (count_from == 0 && !args_from->args_void)){
		/* a() or b() */
		return FUNCARGS_ARE_EQUAL;
	}

	if(args_to->args_old_proto || args_from->args_old_proto)
		return 1;

	if(!(args_to->variadic ? count_to <= count_from : count_to == count_from))
		return FUNCARGS_ARE_MISMATCH_COUNT;

	if(count_to){
		const enum decl_cmp flag = exact ? DECL_CMP_EXACT_MATCH : 0;

		int i;

		for(i = 0; args_to->arglist[i]; i++){
			/* FIXME: this is not an output function */
			char buf[DECL_STATIC_BUFSIZ];

			int eq = fold_type_ref_equal(
					args_to->arglist[i]->ref,
					args_from->arglist[i]->ref,
					&args_from->where, WARN_ARG_MISMATCH, flag,
					"mismatching argument %d %s%s%s(%s <-- %s)",
					i,
					fspel ? "to " : "between declarations ",
					fspel ? fspel : "",
					fspel ? " " : "",
					decl_to_str_r(buf, args_to->arglist[i]),
					decl_to_str(       args_from->arglist[i]));

			if(!eq)
				return FUNCARGS_ARE_MISMATCH_TYPES;
		}
	}

	return FUNCARGS_ARE_EQUAL;
}

funcargs *funcargs_new()
{
	funcargs *r = umalloc(sizeof *funcargs_new());
	where_cc1_current(&r->where);
	return r;
}
