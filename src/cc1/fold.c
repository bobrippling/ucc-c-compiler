#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "fold.h"
#include "fold_sym.h"
#include "sym.h"
#include "../util/platform.h"
#include "const.h"
#include "asm.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "sue.h"
#include "decl.h"

char *curdecl_func_sp;       /* for funcargs-local labels */
stmt *curstmt_flow;          /* for break */
stmt *curstmt_switch;        /* for case + default */

static where asm_struct_enum_where;

void fold_stmt_and_add_to_curswitch(stmt *t)
{
	fold_stmt(t->lhs); /* compound */
	if(!curstmt_switch)
		die_at(&t->expr->where, "not inside a switch statement");
	dynarray_add((void ***)&curstmt_switch->codes, t);

	/* we are compound, copy some attributes */
	t->kills_below_code = t->lhs->kills_below_code;
	/* TODO: copy ->freestanding? */
}

void fold_funcargs_equal(funcargs *args_a, funcargs *args_b, int check_vari, where *w, const char *warn_pre, const char *func_spel)
{
	const int count_a = dynarray_count((void **)args_a->arglist);
	const int count_b = dynarray_count((void **)args_b->arglist);
	int i;

	if(count_a == 0 && !args_a->args_void){
		/* a() */
	}else if(!(check_vari && args_a->variadic ? count_a <= count_b : count_a == count_b)){
		char wbuf[WHERE_BUF_SIZ];
		strcpy(wbuf, where_str(&args_a->where));
		die_at(w, "mismatching argument counts (%d vs %d) for %s (%s)",
				count_a, count_b, func_spel, wbuf);
	}

	if(!count_a)
		return;

	for(i = 0; args_a->arglist[i]; i++){
		char buf_a[DECL_STATIC_BUFSIZ], buf_b[DECL_STATIC_BUFSIZ];
		decl *a, *b;

		a = args_a->arglist[i];
		b = args_b->arglist[i];

		strcpy(buf_a, decl_to_str(a));
		strcpy(buf_b, decl_to_str(b));

		fold_decl_equal(a, b, w, WARN_ARG_MISMATCH,
				"mismatching %s for arg %d in %s (arg %s vs. expr %s)",
				warn_pre, i + 1, func_spel, buf_a, buf_b);
	}
}

void fold_decl_equal(decl *a, decl *b, where *w, enum warning warn,
		const char *errfmt, ...)
{
	if(!decl_equal(a, b, DECL_CMP_ALLOW_VOID_PTR | (fopt_mode & FOPT_STRICT_TYPES ? DECL_CMP_STRICT_PRIMITIVE : 0))){
		int one_struct;
		char buf[DECL_STATIC_BUFSIZ];
		va_list l;

		strcpy(buf, decl_to_str(b));

		cc1_warn_at(w, 0, warn, "%s vs. %s for...", decl_to_str(a), buf);

		one_struct = (!a->desc && a->type->sue && a->type->sue->primitive != type_enum)
			        || (!b->desc && b->type->sue && b->type->sue->primitive != type_enum);

		va_start(l, errfmt);
		cc1_warn_atv(w, one_struct || decl_is_void(a) || decl_is_void(b), warn, errfmt, l);
		va_end(l);
	}
}

int fold_get_sym(expr *e, symtable *stab)
{
	if(!e->sym && e->spel)
		return !!(e->sym = symtab_search(stab, e->spel));
	return 0;
}

void fold_inc_writes_if_sym(expr *e, symtable *stab)
{
	if(fold_get_sym(e, stab))
		e->sym->nwrites++;
}

void fold_expr(expr *e, symtable *stab)
{
	where *old_w;

	fold_get_sym(e, stab);

	old_w = eof_where;
	eof_where = &e->where;
	e->f_fold(e, stab);
	eof_where = old_w;

	const_fold(e); /* fold, then const-propagate */

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());
	UCC_ASSERT(e->tree_type->type->primitive != type_unknown, "unknown type after folding expr %s", e->f_str());
}

void fold_decl_desc(decl_desc *dp, symtable *stab, decl *root)
{
	switch(dp->type){
		case decl_desc_func:
			fold_funcargs(dp->bits.func, stab, decl_spel(root));
			break;

		case decl_desc_array:
		{
			long v;
			fold_expr(dp->bits.array_size, stab);
			if((v = dp->bits.array_size->val.iv.val) < 0)
				die_at(&dp->where, "negative array length %ld", v);

			if(v == 0 && !root->init)
				die_at(&dp->where, "incomplete array");
		}

		case decl_desc_ptr:
			/* TODO: check qual */
			break;
	}

	if(dp->child)
		fold_decl_desc(dp->child, stab, root);
}

void fold_enum(struct_union_enum_st *en, symtable *stab)
{
	sue_member **i;
	int defval = 0;

	for(i = en->members; *i; i++){
		enum_member *m = &(*i)->enum_member;
		expr *e = m->val;

		/* -1 because we can't do dynarray_add(..., 0) */
		if(e == (expr *)-1){

			/*expr_free(e); XXX: memleak */
			where *old_w = eof_where;
			eof_where = &asm_struct_enum_where;
			m->val = expr_new_val(defval++);
			eof_where = old_w;
		}else{
			fold_expr(e, stab);
			if(!const_expr_is_const(e))
				die_at(&e->where, "enum value not constant");
			defval = const_expr_val(e) + 1;
		}
	}
}

int fold_sue(struct_union_enum_st *sue, symtable *stab)
{
	int offset;
	sue_member **i;

	if(sue->primitive == type_enum){
		fold_enum(sue, stab);
		offset = platform_word_size(); /* XXX: assumes enums are 64-bit */
	}else{
		for(offset = 0, i = sue->members; i && *i; i++){
			decl *d = &(*i)->struct_member;

			fold_decl(d, stab);

			if(sue->primitive == type_struct)
				d->struct_offset = offset;
			/* else - union, all offsets are the same */

			if(d->type->sue && decl_ptr_depth(d) == 0){
				if(d->type->sue == sue)
					die_at(&d->where, "nested %s", sue_str(sue));

				offset += fold_sue(d->type->sue, stab);
			}else{
				offset += decl_size(d);
			}
		}
	}

	return offset;
}

void fold_decl_init(decl_init *di, symtable *stab, decl *for_decl)
{
	/* fold + type check for statics + globals */
	if(decl_has_array(for_decl)){ /* FIXME: sufficient for struct A x[] skipping ? */
		/* don't allow scalar inits */
		switch(di->type){
			case decl_init_struct:
			case decl_init_scalar:
				die_at(&for_decl->where, "can't initialise array decl with scalar or struct");

			case decl_init_brace:
				if(decl_has_incomplete_array(for_decl)){
					/* complete the decl */
					decl_desc *dp = decl_array_incomplete(for_decl);
					dp->bits.array_size->val.iv.val = decl_init_len(di);
				}
		}
	}else if(for_decl->type->primitive == type_struct && decl_ptr_depth(for_decl) == 0){
		/* similar to above */
		const int nmembers = sue_nmembers(for_decl->type->sue);

		switch(di->type){
			case decl_init_scalar:
				die_at(&for_decl->where, "can't initialise %s with expression",
						decl_to_str(for_decl));

			case decl_init_struct:
			{
				const int ninits = dynarray_count((void **)di->bits.subs);
				int i;

				/* for_decl is a struct - the above else-if */

				for(i = 0; i < ninits; i++){
					struct decl_init_sub *sub = di->bits.subs[i];
					sub->member = struct_union_member_find(for_decl->type->sue, sub->spel, &for_decl->where);
				}

				fprintf(stderr, "checked decl struct init for %s\n", for_decl->spel);
				break;
			}

			case decl_init_brace:
				if(for_decl->type->sue){
					/* init struct with braces */
					const int nbraces  = dynarray_count((void **)di->bits.subs);

					if(nbraces > nmembers)
						die_at(&for_decl->where, "too many initialisers for %s", decl_to_str(for_decl));

					/* folded below */

				}else{
					die_at(&for_decl->where, "initialisation of incomplete struct");
				}
				break;
		}
	}


	switch(di->type){
		case decl_init_scalar:
			if(for_decl->type->store == store_static || (for_decl->sym && for_decl->sym->type == sym_global)){
				char buf_a[DECL_STATIC_BUFSIZ], buf_b[DECL_STATIC_BUFSIZ];
				expr *init_exp = di->bits.expr;

				fold_expr(init_exp, stab);

				/* TODO: better error desc - init of subobject, etc */
				strcpy(buf_a, decl_to_str(for_decl));
				strcpy(buf_b, decl_to_str(init_exp->tree_type));

				fold_decl_equal(for_decl, init_exp->tree_type, &for_decl->where, WARN_ASSIGN_MISMATCH,
						"mismatching initialisation for %s (%s vs. %s)",
						decl_spel(for_decl), buf_a, buf_b);

				if(const_fold(init_exp) && !const_expr_is_const(init_exp)){
					/* global/static + not constant */
					/* allow identifiers if the identifier is also static */

					if(!expr_kind(init_exp, identifier) || init_exp->tree_type->type->store != store_static){
						die_at(&init_exp->where, "not a constant expression for %s init - %s",
								for_decl->spel, init_exp->f_str());
					}
				}
			}
			break;

		case decl_init_struct:
		case decl_init_brace:
		{
			decl_init_sub *s;
			int i;

			/* recursively fold */
			for(i = 0; (s = di->bits.subs[i]); i++){
				decl *d;
				if(!(d = s->member)){
					/* temp decl from the array type */
					d = decl_ptr_depth_dec(decl_copy(for_decl), &for_decl->where);
					fprintf(stderr, "implicit for_decl %s\n", decl_to_str(d));
				}

				fold_decl_init(s->init, stab, d);

				if(d != s->member)
					decl_free(d);
			}
			break;
		}
	}

}

void fold_decl(decl *d, symtable *stab)
{
	decl_desc *dp;

	/* typedef / __typeof folding */
	while(d->type->typeof){
		/* get the typedef decl from t->decl->tree_type */
		const enum type_qualifier old_qual  = d->type->qual;
		const enum type_storage   old_store = d->type->store;
		decl *from;
		expr *type_exp;

		type_exp = d->type->typeof;

		fold_expr(type_exp, stab);
		decl_free(type_exp->tree_type);

		/* either get the typeof() from the decl or the expr type */
		from = d->type->typeof->decl;
		if(!from)
			from = d->type->typeof->expr->tree_type;

		UCC_ASSERT(from, "no decl for typeof/typedef fold: "
				".decl = %p, .expr->tt = %p",
				d->type->typeof->decl,
				d->type->typeof->expr->tree_type);

		type_exp->tree_type = decl_copy(from);

		/* type */
		memcpy(d->type, from->type, sizeof *d->type);
		d->type->qual  |= old_qual;
		d->type->store  = old_store;

		/* decl */
		if(from->desc){
			decl_desc *ins = decl_desc_copy(from->desc);

			decl_desc_append(&ins, d->desc);
			d->desc = ins;
		}
	}
	decl_desc_link(d);

	UCC_ASSERT(d->type && d->type->store != store_typedef, "typedef store after tdef folding");

	/* check for array of funcs, func returning array */
	for(dp = decl_desc_tail(d); dp && dp->parent_desc; dp = dp->parent_desc){
		if(dp->parent_desc->type == decl_desc_func){
			if(dp->type == decl_desc_array)
				die_at(&dp->where, "can't have an array of functions");
			else if(dp->type == decl_desc_func)
				die_at(&dp->where, "can't have a function returning a function");
		}
	}

	switch(d->type->primitive){
		case type_void:
			if(!decl_ptr_depth(d) && !decl_is_callable(d) && d->spel)
				die_at(&d->where, "can't have a void variable - %s (%s)", decl_spel(d), decl_to_str(d));
			break;

		case type_enum:
		case type_struct:
		case type_union:
			if(sue_incomplete(d->type->sue) && !decl_ptr_depth(d))
				die_at(&d->where, "use of %s%s%s",
						type_to_str(d->type),
						decl_spel(d) ?     " " : "",
						decl_spel(d) ? decl_spel(d) : "");
			break;

		case type_int:
		case type_char:
			break;

		case type_unknown:
			ICE("unknown type");
	}

	/*
	 * now we've folded, check for restrict
	 * since typedef int *intptr; intptr restrict a; is valid
	 */
	if(d->desc){
		fold_decl_desc(d->desc, stab, d);
	}else if(d->type->qual & qual_restrict){
		die_at(&d->where, "restrict on non-pointer type %s%s%s",
				type_to_str(d->type),
				decl_spel(d) ? " " : "",
				decl_spel(d) ? decl_spel(d) : "");
	}

	if(decl_is_func(d)){
		if(d->type->store == store_register)
			die_at(&d->where, "register storage for function");
	}else{
		if(d->type->is_inline)
			warn_at(&d->where, "inline on non-function%s%s",
					d->spel ? " " : "",
					d->spel ? d->spel : "");
	}

	if(d->init){
		if(d->type->store == store_extern)
			die_at(&d->where, "externs can't be initalised");

		fold_decl_init(d->init, stab, d);
	}
}

void fold_decl_global(decl *d, symtable *stab)
{
	if(d->type->store == store_extern){
		/* we have an extern, check if it's overridden */
		char *const spel = decl_spel(d);
		decl **dit;

		for(dit = stab->decls; dit && *dit; dit++){
			decl *d2 = *dit;
			if(!strcmp(decl_spel(d2), spel) && d2->type->store != store_extern){
				/* found an override */
				d->ignore = 1;
				break;
			}
		}
	}

	fold_decl(d, stab);
}

void fold_symtab_scope(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++)
		fold_sue(*sit, stab);
}

void fold_test_expr(expr *e, const char *stmt_desc)
{
	if(!decl_ptr_depth(e->tree_type) && e->tree_type->type->primitive == type_void)
		die_at(&e->where, "%s requires non-void expression", stmt_desc);

	if(!e->in_parens && expr_kind(e, assign))
		cc1_warn_at(&e->where, 0, WARN_TEST_ASSIGN, "testing an assignment in %s", stmt_desc);

	fold_disallow_st_un(e, stmt_desc);
}

void fold_disallow_st_un(expr *e, const char *desc)
{
	if(!decl_ptr_depth(e->tree_type) && decl_is_struct_or_union(e->tree_type)){
		die_at(&e->where, "%s involved in %s",
				sue_str(e->tree_type->type->sue),
				desc);
	}
}

void fold_stmt(stmt *t)
{
	UCC_ASSERT(t->symtab->parent, "symtab has no parent");

	t->f_fold(t);
}

void fold_funcargs(funcargs *fargs, symtable *stab, char *context)
{
	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;

		for(i = 0; fargs->arglist[i]; i++){
			decl *const d = fargs->arglist[i];

			/* convert any array definitions to pointers */
			decl_conv_array_ptr(d); /* must be before the decl is folded */

			fold_decl(d, stab);

			if(type_store_static_or_extern(d->type->store)){
				const char *sp = decl_spel(d);
				die_at(&fargs->where, "argument %d %s%s%sin function \"%s\" is static or extern",
						i + 1,
						sp ? "(" : "",
						sp ? sp  : "",
						sp ? ") " : "",
						context);
			}
		}
	}
}

void fold_func(decl *func_decl, symtable *globs)
{
	curdecl_func_sp = decl_spel(func_decl);

	if(func_decl->func_code){
		funcargs *fargs;
		int nargs, i;

		fargs = decl_desc_tail(func_decl)->bits.func;

		if(fargs->arglist){
			for(nargs = 0; fargs->arglist[nargs]; nargs++);
			/* add args backwards, since we push them onto the stack backwards - still need to do this here? */
			for(i = nargs - 1; i >= 0; i--){
				if(!decl_spel(fargs->arglist[i]))
					die_at(&fargs->where, "function \"%s\" has unnamed arguments", curdecl_func_sp);
				else
					SYMTAB_ADD(func_decl->func_code->symtab, fargs->arglist[i], sym_arg);
			}
		}

		symtab_set_parent(func_decl->func_code->symtab, globs);

		fold_stmt(func_decl->func_code);

		curdecl_func_sp = NULL;
	}
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	int i;

	{
		extern const char *current_fname;
		memset(&asm_struct_enum_where, 0, sizeof asm_struct_enum_where);
		asm_struct_enum_where.fname = current_fname;
	}

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *df;
		funcargs *fargs;
		where *old_w;

		old_w = eof_where;
		eof_where = &asm_struct_enum_where;

		df = decl_new();
		decl_set_spel(df, ustrdup(ASM_INLINE_FNAME));

		df->type->primitive = type_int;

		fargs = funcargs_new();
		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[0] = decl_new();
		fargs->arglist[1] = NULL;
		fargs->arglist[0]->type->primitive = type_char;
		fargs->arglist[0]->type->qual      = qual_const;
		fargs->arglist[0]->desc            = decl_desc_ptr_new(fargs->arglist[0], NULL);

		df->desc = decl_desc_func_new(df, NULL);
		df->desc->bits.func = fargs;

		symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);

		eof_where = old_w;
	}

	fold_symtab_scope(globs);

	for(i = 0; D(i); i++)
		if(D(i)->sym)
			ICE("%s: sym (%p) already set for global \"%s\"", where_str(&D(i)->where), D(i)->sym, decl_spel(D(i)));

	for(;;){
		int i;

		/* find the next sym (since we can prepend, start at 0 each time */
		for(i = 0; D(i); i++)
			if(!D(i)->sym)
				break;

		if(!D(i))
			break; /* finished */

		D(i)->sym = sym_new(D(i), sym_global);

		fold_decl_global(D(i), globs);

		if(decl_is_func(D(i)))
			fold_func(D(i), globs);
	}

	/*
	 * change int x(); to extern int x();
	 * if there is no decl->func_code for "x"
	 *
	 * otherwise, ignore it (since we have a func code for it elsewhere)
	 * e.g. int x(); int x(){}
	 */
	for(i = 0; D(i); i++){
		char *const spel_i = decl_spel(D(i));

		if(decl_is_func(D(i)) && !D(i)->func_code){
			int found = 0;
			int j;

			for(j = 0; D(j); j++){
				if(j != i && !strcmp(spel_i, decl_spel(D(j)))){
					/* D(i) is a prototype, check the args match D(j) */
					fold_funcargs_equal(decl_funcargs(D(i)), decl_funcargs(D(j)),
							0, &D(j)->where, "type", decl_spel(D(j)));

					if(!decl_equal(D(i), D(j), DECL_CMP_STRICT_PRIMITIVE)){
						char wbuf[WHERE_BUF_SIZ];
						strcpy(wbuf, where_str(&D(j)->where));
						die_at(&D(i)->where, "mismatching return types for function %s (%s)", decl_spel(D(i)), wbuf);
					}

					if(D(j)->func_code){
						/* D(j) is the implementation */
						D(i)->ignore = 1;
						found = 1;
					}
					break;
				}
			}

			if(!found){
				D(i)->type->store = store_extern;
				/*cc1_warn_at(&f->where, 0, WARN_EXTERN_ASSUME, "assuming \"%s\" is extern", func_decl_spel(decl));*/
			}
		}
	}

	/* static assertions */
	{
		static_assert **i;
		for(i = globs->static_asserts; i && *i; i++){
			static_assert *sa = *i;
			fold_expr(sa->e, sa->scope);
			if(const_fold(sa->e))
				die_at(&sa->e->where, "static assert: not a constant expression (%s)", sa->e->f_str());
			if(!sa->e->val.iv.val)
				die_at(&sa->e->where, "static assertion failure: %s", sa->s);
		}
	}

#undef D
}
