#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/dynarray.h"
#include "../util/util.h"

#include "data_structs.h"
#include "expr.h"
#include "decl.h"
#include "sym.h"

#include "tokenise.h"
#include "tokconv.h"
#include "parse.h"
#include "parse_type.h"

#include "pass1.h"


static void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	int fold_had_error = 0;
	extern const char *current_fname;
	int i;

	memset(&asm_struct_enum_where, 0, sizeof asm_struct_enum_where);
	asm_struct_enum_where.fname = current_fname;

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *df;
		funcargs *fargs;
		const where *old_w;

		old_w = eof_where;
		eof_where = &asm_struct_enum_where;

		df = decl_new();
		df->spel = ustrdup(ASM_INLINE_FNAME);

		fargs = funcargs_new();
		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[1] = NULL;

		/* const char * */
		(fargs->arglist[0] = decl_new())->ref = type_ref_new_ptr(
				type_ref_new_type_qual(type_char, qual_const),
				qual_none);

		df->ref = type_ref_new_func(type_ref_cached_INT(), fargs);

		ICE("__asm__ symtable");
		/*symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);*/

		eof_where = old_w;
	}

	fold_symtab_scope(globs, NULL);

	if(globs->decls){
		for(i = 0; D(i); i++)
			fold_decl_global(D(i), globs);

		fold_merge_tenatives(globs);
	}

	if(fold_had_error)
		exit(1);
#undef D
}

static void fold_static_asserts(static_assert **sas)
{
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

void parse_and_fold(symtable_global *globals)
{
	symtable_gasm **last_gasms = NULL;

	current_scope = &globals->stab;

	type_ref_init(current_scope);

	for(;;){
		int cont = 0;
		decl **new = NULL;

		parse_decls_single_type(
				  DECL_MULTI_CAN_DEFAULT
				| DECL_MULTI_ACCEPT_FUNC_CODE
				| DECL_MULTI_ALLOW_STORE
				| DECL_MULTI_ALLOW_ALIGNAS,
				current_scope,
				&new);

		if(new){
			symtable_gasm **i;

			for(i = last_gasms; i && *i; i++)
				(*i)->before = *new;
			dynarray_free(symtable_gasm **, &last_gasms, NULL);
			dynarray_free(decl **, &new, NULL);
		}

		/* global asm */
		while(accept(token_asm)){
			symtable_gasm *g = parse_gasm();

			dynarray_add(&last_gasms, g);
			dynarray_add(&globals->gasms, g);
			cont = 1;
		}

		/* fold */
		fold(

		if(!cont)
			break;
	}

	dynarray_free(symtable_gasm **, &last_gasms, NULL);

	EAT(token_eof);

	if(parse_had_error)
		exit(1);

	current_scope->static_asserts = static_asserts;

	UCC_ASSERT(!current_scope->parent, "scope leak during parse");
}
