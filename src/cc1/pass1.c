#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/where.h"
#include "../util/dynarray.h"
#include "../util/util.h"

#include "expr.h"
#include "decl.h"
#include "sym.h"
#include "fold_sym.h"

#include "tokenise.h"
#include "tokconv.h"

#include "parse_type.h"
#include "parse_stmt.h"

#include "cc1.h"
#include "fold.h"
#include "const.h"
#include "type_nav.h"

#include "pass1.h"

static void link_gasms(symtable_gasm ***plast_gasms, decl *prev)
{
	symtable_gasm **i;

	for(i = *plast_gasms; i && *i; i++)
		(*i)->before = prev;

	dynarray_free(symtable_gasm **, *plast_gasms, NULL);
}

static int parse_add_gasms(symtable_gasm ***plast_gasms)
{
	int r = 0;
	while(accept(token_asm)){
		symtable_gasm *g = parse_gasm();
		if(g)
			dynarray_add(plast_gasms, g);
		r = 1;
	}
	return r;
}

int parse_and_fold(symtable_global *globals)
{
	symtable_gasm **last_gasms = NULL;

	while(curtok != token_eof){
		where semi;
		decl **new = NULL;
		decl **di;
		int cont;

		cont = parse_decl_group(
				  DECL_MULTI_CAN_DEFAULT
				| DECL_MULTI_ACCEPT_FUNC_CODE
				| DECL_MULTI_ALLOW_STORE
				| DECL_MULTI_ALLOW_ALIGNAS,
				/*newdecl:*/1,
				&globals->stab,
				&globals->stab, &new);

		/* global struct layout-ing */
		symtab_fold_sues(&globals->stab);

		if(new){
			link_gasms(&last_gasms, *new);

			/* fold what we got */
			for(di = new; di && *di; di++)
				fold_decl_global(*di, &globals->stab);

			dynarray_free(decl **, new, NULL);

			cont = 1;
		}

		cont |= parse_add_gasms(&last_gasms);
		dynarray_add_array(&globals->gasms, last_gasms);

		while(accept_where(token_semicolon, &semi)){
			cc1_warn_at(&semi, parse_extra_semi, "extra ';' at global scope");
			cont = 1;
		}

		if(!cont)
			break;
	}

	EAT(token_eof);

	symtab_fold_sues(&globals->stab); /* superflous except for empty
	                                  * files/trailing struct defs */
	symtab_fold_decls(&globals->stab); /* check for dups */
	symtab_check_rw(&globals->stab); /* basic static analysis */
	symtab_check_static_asserts(&globals->stab);

	fold_merge_tenatives(&globals->stab);

	dynarray_free(symtable_gasm **, last_gasms, NULL);

	UCC_ASSERT(!globals->stab.parent, "scope leak during parse");

	return parse_had_error || fold_had_error;
}
