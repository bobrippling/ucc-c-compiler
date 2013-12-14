#include <string.h>
#include "ops.h"
#include "expr_identifier.h"
#include "../out/asm.h"
#include "../sue.h"
#include "expr_addr.h"

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
		if(sym->type == sym_global || decl_store_static_or_extern(d->store)){
			CONST_FOLD_LEAF(k);

			k->type = CONST_ADDR_OR_NEED(d);

			/*
			 * don't use e->spel
			 * static int i;
			 * int x;
			 * x = i; // e->spel is "i". sym->decl->spel is "func_name.static_i"
			 */
			k->bits.addr.bits.lbl = decl_asm_spel(sym->decl);

			k->bits.addr.is_lbl = 1;
			k->offset = 0;
		}
	}
}

static void gen_expr_identifier_lea(expr *e)
{
	out_push_sym(e->bits.ident.sym);
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

			expr_mutate_str(e, sp, strlen(sp) + 1, /*wide:*/0, &e->where);
			/* +1 - take the null byte */
			e->bits.strlit.is_func = 1;

			FOLD_EXPR(e, stab);
		}else{
			/* check for an enum */
			struct_union_enum_st *sue;
			enum_member *m;

			enum_member_search(&m, &sue, stab, sp);

			if(!m)
				die_at(&e->where, "undeclared identifier \"%s\"", sp);

			expr_mutate_wrapper(e, val);

			e->bits.iv = m->val->bits.iv;
			FOLD_EXPR(e, stab);

			e->tree_type = type_ref_new_type(
					type_new_primitive_sue(type_enum, sue));
		}
		return;
	}

	e->tree_type = sym->decl->ref;

	/* set if lvalue - expr_is_lval() checks for arrays */
	e->f_lea =
		type_ref_is(e->tree_type, type_ref_func)
		? NULL
		: gen_expr_identifier_lea;


	if(sym->type == sym_local
	&& !decl_store_static_or_extern(sym->decl->store)
	&& !DECL_IS_ARRAY(sym->decl)
	&& !DECL_IS_S_OR_U(sym->decl)
	&& !DECL_IS_FUNC(sym->decl)
	&& sym->nwrites == 0
	&& !sym->decl->init)
	{
		cc1_warn_at(&e->where, 0, WARN_READ_BEFORE_WRITE, "\"%s\" uninitialised on read", sp);
		sym->nwrites = 1; /* silence future warnings */
	}

	/* this is cancelled by expr_assign in the case we fold for an assignment to us */
	sym->nreads++;

	if(!sym->func)
		sym->func = in_fn;
}

void gen_expr_str_identifier(expr *e)
{
	idt_printf("identifier: \"%s\" (sym %p)\n", e->bits.ident.spel, (void *)e->bits.ident.sym);
}

void gen_expr_identifier(expr *e)
{
	sym *sym = e->bits.ident.sym;

	if(DECL_IS_FUNC(sym->decl))
		out_push_sym(sym);
	else
		out_push_sym_val(sym);
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

void gen_expr_style_identifier(expr *e)
{
	stylef("%s", e->bits.ident.spel);
}
