#include <string.h>
#include <assert.h>
#include "ops.h"
#include "expr_identifier.h"
#include "../out/asm.h"
#include "../sue.h"
#include "expr_addr.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../str.h"

#include "expr_string.h"

const char *str_expr_identifier()
{
	return "identifier";
}

static void fold_const_expr_identifier(expr *e, consty *k)
{
	/*
	 * if we are an array identifier, we are constant:
	 * int x[];
	 */
	int set_no = 1;

	/* may not have e->sym if we're the struct-member-identifier */
	switch(e->bits.ident.type){
		case IDENT_NORM:
		{
			sym *sym = e->bits.ident.bits.ident.sym;

			if(sym && sym->decl){
				decl *const d = sym->decl;

				/* only a constant if global/static/extern */
				if(decl_store_duration_is_static(d) && !attribute_present(d, attr_weak)){
					CONST_FOLD_LEAF(k);

					k->type = CONST_ADDR_OR_NEED(d);

					k->bits.addr.bits.lbl = decl_asm_spel(sym->decl);

					k->bits.addr.is_lbl = 1;
					k->offset = 0;

					set_no = 0;
				}
			}
			break;
		}
		case IDENT_ENUM:
			if(e->bits.ident.bits.enum_mem->val == (void *)-1){
				/* part-way through processing an enum, we reference one in the
				 * future that hasn't had its value set. invalid, error caught later
				 */
				return;
			}

			const_fold(e->bits.ident.bits.enum_mem->val, k);
			set_no = 0;
			break;
	}

	if(set_no)
		CONST_FOLD_NO(k, e);
}

static int attempt_func_keyword(expr *expr_ident, symtable *stab)
{
	const char *sp = expr_ident->bits.ident.bits.ident.spel;
	int std = 1;

	if(!strcmp(sp, "__func__") || (std = 0, !strcmp(sp, "__FUNCTION__"))){
		char *fnsp;
		struct cstring *cstr;
		decl *in_fn = symtab_func(stab);

		if(!std){
			cc1_warn_at(
					&expr_ident->where, gnu__function,
					"use of GNU __FUNCTION__");
		}

		if(!in_fn){
			cc1_warn_at(&expr_ident->where,
					x__func__outsidefn,
					"%s is not defined outside of functions",
					sp);

			fnsp = "";
		}else{
			fnsp = in_fn->spel;
		}

		cstr = cstring_new(CSTRING_ASCII, fnsp, strlen(fnsp));

		expr_mutate_str(
				expr_ident,
				cstr,
				&expr_ident->where,
				stab);

		/* +1 - take the null byte */
		expr_ident->bits.strlit.is_func = 2 - std;

		FOLD_EXPR(expr_ident, stab);
		return 1;
	}

	return 0;
}

static int find_identifier(expr *expr_ident, symtable *stab)
{
	char *sp = expr_ident->bits.ident.bits.ident.spel;
	int found = 0;
	struct symtab_entry ent;

	found = symtab_search(stab, sp, NULL, &ent);

	if(!found)
		return attempt_func_keyword(expr_ident, stab);

	switch(ent.type){
		case SYMTAB_ENT_ENUM:
		{
			expr_ident->bits.ident.type = IDENT_ENUM;
			expr_ident->bits.ident.bits.enum_mem = ent.bits.enum_member.memb;

			expr_ident->tree_type = type_nav_int_enum(
					cc1_type_nav, ent.bits.enum_member.sue);

			expr_ident->f_islval = NULL;
			break;
		}

		case SYMTAB_ENT_DECL:
		{
			sym *const sym = ent.bits.decl->sym;

			if(!sym)
				return 0; /* not found, e.g. __auto_type x = x; */

			expr_ident->bits.ident.bits.ident.sym = sym;

			/* prevent typedef */
			if(sym && STORE_IS_TYPEDEF(sym->decl->store)){
				warn_at_print_error(&expr_ident->where,
						"use of typedef-name '%s' as expression",
						sp);
				fold_had_error = 1;

				/* prevent warnings lower down */
				sym->nwrites++;
			}

			expr_ident->bits.ident.type = IDENT_NORM;
			expr_ident->tree_type = type_attributed(sym->decl->ref, sym->decl->attr);

			decl_use(sym->decl);

			/* set if lvalue */
			if(type_is(expr_ident->tree_type, type_func))
				expr_ident->f_islval = NULL;

			if(sym->type == sym_local
			&& !decl_store_duration_is_static(sym->decl)
			&& !type_is(sym->decl->ref, type_array)
			&& !type_is(sym->decl->ref, type_func)
			&& !type_is_s_or_u(sym->decl->ref)
			&& sym->nwrites == 0
			&& !sym->decl->bits.var.init.dinit)
			{
				cc1_warn_at(&expr_ident->where, uninitialised,
						"\"%s\" uninitialised on read", sp);
				sym->nwrites = 1; /* silence future warnings */
			}

			/* this is cancelled by expr_assign in the case we fold for an assignment to us */
			sym->nreads++;
			break;
		}
	}

	return 1;
}

void fold_expr_identifier(expr *e, symtable *stab)
{
	if(!find_identifier(e, stab)){
		char *sp = e->bits.ident.bits.ident.spel;
		warn_at_print_error(&e->where, "undeclared identifier \"%s\"", sp);
		fold_had_error = 1;
		e->tree_type = type_nav_btype(cc1_type_nav, type_int);
	}
}

void dump_expr_identifier(const expr *e, dump *ctx)
{
	const char *desc = NULL;
	const char *namespace = "";

	switch(e->bits.ident.type){
		case IDENT_NORM:
			desc = "identifier";
			break;

		case IDENT_ENUM:
		{
			struct_union_enum_st *sue = type_is_enum(e->tree_type);
			if(!sue->anon){
				namespace = sue->spel;
				assert(namespace);
			}
			desc = "enum constant";
			break;
		}
	}

	dump_desc_expr_newline(ctx, desc, e, 0);
	dump_printf_indent(ctx, 0, " %s%s%s\n",
			namespace,
			*namespace ? "::" : "",
			e->bits.ident.bits.ident.spel);
}

const out_val *gen_expr_identifier(const expr *e, out_ctx *octx)
{
	switch(e->bits.ident.type){
		case IDENT_NORM:
			return out_new_sym(octx, e->bits.ident.bits.ident.sym);

		case IDENT_ENUM:
			return out_new_l(octx, e->tree_type,
					const_fold_val_i(e->bits.ident.bits.enum_mem->val));
	}
	assert(0);
}

void mutate_expr_identifier(expr *e)
{
	e->f_const_fold  = fold_const_expr_identifier;
	e->f_islval = expr_is_lval_always;
}

expr *expr_new_identifier(char *sp)
{
	expr *e = expr_new_wrapper(identifier);
	UCC_ASSERT(sp, "NULL spel for identifier");
	e->bits.ident.bits.ident.spel = sp;
	return e;
}

const out_val *gen_expr_style_identifier(const expr *e, out_ctx *octx)
{
	switch(e->bits.ident.type){
		case IDENT_NORM:
			stylef("%s", e->bits.ident.bits.ident.spel);
			break;
		case IDENT_ENUM:
			stylef("%s", e->bits.ident.bits.enum_mem->spel);
			break;
	}
	UNUSED_OCTX();
}

const char *expr_ident_spel(expr *e)
{
	switch(e->bits.ident.type){
		case IDENT_NORM: return e->bits.ident.bits.ident.spel;
		case IDENT_ENUM: return e->bits.ident.bits.enum_mem->spel;
	}
	assert(0);
}
