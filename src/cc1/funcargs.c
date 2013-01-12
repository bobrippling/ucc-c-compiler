#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "decl.h"
#include "funcargs.h"

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

funcargs *funcargs_new()
{
	funcargs *r = umalloc(sizeof *funcargs_new());
	where_new(&r->where);
	return r;
}
