#include <string.h>
#include "ops.h"
#include "expr_identifier.h"
#include "../out/asm.h"
#include "../sue.h"
#include "expr_addr.h"
#include "../type_is.h"
#include "../type_nav.h"

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
	sym *sym;

	k->type = CONST_NO;

	/* may not have e->sym if we're the struct-member-identifier */
	if((sym = e->bits.ident.sym) && sym->decl){
		decl *const d = sym->decl;

		/* only a constant if global/static/extern */
		if(decl_store_duration_is_static(d) && !attribute_present(d, attr_weak)){
			CONST_FOLD_LEAF(k);

			k->type = CONST_ADDR_OR_NEED(d);

			k->bits.addr.bits.lbl = decl_asm_spel(sym->decl);

			k->bits.addr.is_lbl = 1;
			k->offset = 0;
		}
	}
}

static const out_val *gen_expr_identifier_lea(expr *e, out_ctx *octx)
{
	return out_new_sym(octx, e->bits.ident.sym);
}

void fold_expr_identifier(expr *e, symtable *stab)
{
	char *sp = e->bits.ident.spel;
	sym *sym = e->bits.ident.sym;
	decl *in_fn = symtab_func(stab);

	if(sp && !sym)
		e->bits.ident.sym = sym = symtab_search(stab, sp);

	/* special cases */
	if(!sym){
		if(!strcmp(sp, "__func__")){
			char *sp;

			if(!in_fn){
				warn_at(&e->where, "__func__ is not defined outside of functions");

				sp = "";
			}else{
				sp = in_fn->spel;
			}

			expr_mutate_str(e, sp, strlen(sp) + 1, /*wide:*/0, &e->where, stab);
			/* +1 - take the null byte */
			e->bits.strlit.is_func = 1;

			FOLD_EXPR(e, stab);
		}else{
			/* check for an enum */
			struct_union_enum_st *sue;
			enum_member *m;

			enum_member_search(&m, &sue, stab, sp);

			if(!m){
				warn_at_print_error(&e->where, "undeclared identifier \"%s\"", sp);
				fold_had_error = 1;
				e->tree_type = type_nav_btype(cc1_type_nav, type_int);
				return;
			}

			expr_mutate_wrapper(e, val);

			e->bits.num = m->val->bits.num;
			FOLD_EXPR(e, stab);

			e->tree_type = type_nav_suetype(cc1_type_nav, sue);
		}
		return;
	}

	e->tree_type = sym->decl->ref;

	/* set if lvalue - expr_is_lval() checks for arrays */
	e->f_lea =
		type_is(e->tree_type, type_func)
		? NULL
		: gen_expr_identifier_lea;


	if(sym->type == sym_local
	&& !decl_store_duration_is_static(sym->decl)
	&& !type_is(sym->decl->ref, type_array)
	&& !type_is(sym->decl->ref, type_func)
	&& !type_is_s_or_u(sym->decl->ref)
	&& sym->nwrites == 0
	&& !sym->decl->bits.var.init)
	{
		cc1_warn_at(&e->where, 0, WARN_READ_BEFORE_WRITE, "\"%s\" uninitialised on read", sp);
		sym->nwrites = 1; /* silence future warnings */
	}

	/* this is cancelled by expr_assign in the case we fold for an assignment to us */
	sym->nreads++;
}

const out_val *gen_expr_str_identifier(expr *e, out_ctx *octx)
{
	idt_printf("identifier: \"%s\" (sym %p)\n", e->bits.ident.spel, (void *)e->bits.ident.sym);
	UNUSED_OCTX();
}

const out_val *gen_expr_identifier(expr *e, out_ctx *octx)
{
	sym *sym = e->bits.ident.sym;

	if(type_is(sym->decl->ref, type_func)){
		UCC_ASSERT(sym->type != sym_arg, "function as argument?");

		return out_new_sym(octx, sym);
	}else{
		return out_new_sym_val(octx, sym);
	}
}

void mutate_expr_identifier(expr *e)
{
	e->f_const_fold  = fold_const_expr_identifier;
}

expr *expr_new_identifier(char *sp)
{
	expr *e = expr_new_wrapper(identifier);
	UCC_ASSERT(sp, "NULL spel for identifier");
	e->bits.ident.spel = sp;
	return e;
}

const out_val *gen_expr_style_identifier(expr *e, out_ctx *octx)
{
	stylef("%s", e->bits.ident.spel);
	UNUSED_OCTX();
}
