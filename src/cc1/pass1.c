#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/where.h"
#include "../util/dynarray.h"
#include "../util/util.h"

#include "data_structs.h"
#include "expr.h"
#include "decl.h"
#include "sym.h"
#include "fold_sym.h"

#include "tokenise.h"
#include "tokconv.h"
#include "parse.h"
#include "parse_type.h"

#include "cc1.h"
#include "fold.h"
#include "const.h"

#include "pass1.h"

static void link_gasms(symtable_gasm ***plast_gasms, decl *prev)
{
	symtable_gasm **i;

	for(i = *plast_gasms; i && *i; i++)
		(*i)->before = prev;

	dynarray_free(symtable_gasm **, plast_gasms, NULL);
}

static int parse_add_gasms(symtable_gasm ***plast_gasms)
{
	int r = 0;
	while(accept(token_asm)){
		dynarray_add(plast_gasms, parse_gasm());
		r = 1;
	}
	return r;
}

void parse_and_fold(symtable_global *globals)
{
	symtable_gasm **last_gasms = NULL;

	current_scope = &globals->stab;

	type_init(current_scope);

	while(curtok != token_eof){
		decl **new = NULL;
		decl **di;
		int cont;

		cont = parse_decls_single_type(
				  DECL_MULTI_CAN_DEFAULT
				| DECL_MULTI_ACCEPT_FUNC_CODE
				| DECL_MULTI_ALLOW_STORE
				| DECL_MULTI_ALLOW_ALIGNAS,
				/*newdecl:*/1,
				current_scope,
				&new);

		/* global struct layout-ing */
		symtab_fold_sues(current_scope);

		if(new){
			link_gasms(&last_gasms, *new);

			/* fold what we got */
			for(di = new; di && *di; di++)
				fold_decl_global(*di, current_scope);

			dynarray_free(decl **, &new, NULL);

			cont = 1;
		}

		cont |= parse_add_gasms(&last_gasms);
		dynarray_add_array(&globals->gasms, last_gasms);

		if(!cont)
			break;
	}

	EAT(token_eof);

	symtab_fold_sues(current_scope); /* superflous except for empty
	                                  * files/trailing struct defs */
	symtab_fold_decls(current_scope); /* check for dups */
	symtab_check_rw(current_scope); /* basic static analysis */
	symtab_check_static_asserts(current_scope);

	fold_merge_tenatives(current_scope);

	dynarray_free(symtable_gasm **, &last_gasms, NULL);

	if(parse_had_error || fold_had_error)
		exit(1);

	UCC_ASSERT(!current_scope->parent, "scope leak during parse");
}
