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

static void fold_check_static_asserts(static_assert **sas)
{
	/* TODO #4: do this for each scope, from fold_symtab_scope() or symtab_fold()
	 * TODO #5:                merge these functions? ^
	 */
	static_assert **i;
	for(i = sas; i && *i; i++){
		static_assert *sa = *i;
		consty k;

		FOLD_EXPR(sa->e, sa->scope);
		if(!type_ref_is_integral(sa->e->tree_type))
			DIE_AT(&sa->e->where,
					"static assert: not an integral expression (%s)",
					sa->e->f_str());

		const_fold(sa->e, &k);

		if(k.type != CONST_VAL)
			DIE_AT(&sa->e->where,
					"static assert: not an integer constant expression (%s)",
					sa->e->f_str());

		if(!k.bits.iv.val)
			DIE_AT(&sa->e->where, "static assertion failure: %s", sa->s);

		if(fopt_mode & FOPT_SHOW_STATIC_ASSERTS){
			fprintf(stderr, "%s: static assert passed: %s-expr, msg: %s\n",
					where_str(&sa->e->where), sa->e->f_str(), sa->s);
		}
	}
}

static void link_gasms(symtable_gasm ***plast_gasms, decl *prev)
{
	symtable_gasm **i;

	for(i = *plast_gasms; i && *i; i++)
		(*i)->before = prev;

	dynarray_free(symtable_gasm **, plast_gasms, NULL);
}

static void parse_add_gasms(symtable_gasm ***plast_gasms)
{
	while(accept(token_asm))
		dynarray_add(plast_gasms, parse_gasm());
}

void parse_and_fold(symtable_global *globals)
{
	symtable_gasm **last_gasms = NULL;

	current_scope = &globals->stab;

	type_ref_init(current_scope);

	while(curtok != token_eof){
		decl **new = NULL;
		decl **di;

		parse_decls_single_type(
				  DECL_MULTI_CAN_DEFAULT
				| DECL_MULTI_ACCEPT_FUNC_CODE
				| DECL_MULTI_ALLOW_STORE
				| DECL_MULTI_ALLOW_ALIGNAS,
				current_scope,
				&new);

		if(new){
			link_gasms(&last_gasms, *new);

			/* fold what we got */
			for(di = new; di && *di; di++)
				fold_decl_global(*di, current_scope);

			dynarray_free(decl **, &new, NULL);
		}

		parse_add_gasms(&last_gasms);
		dynarray_add_array(&globals->gasms, last_gasms);
	}

	EAT(token_eof);

	/* TODO #1: this needs calling individually when we complete
	 * a struct, or block scope (or global scope) etc */
	symtab_make_syms_and_inits(current_scope, NULL);

	fold_merge_tenatives(current_scope);

	dynarray_free(symtable_gasm **, &last_gasms, NULL);

	if(parse_had_error)
		exit(1);

	/* TODO: related to TODO #4 */
	current_scope->static_asserts = static_asserts;
	fold_check_static_asserts(globals->stab.static_asserts);

	UCC_ASSERT(!current_scope->parent, "scope leak during parse");
}
