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

#define DECL_IS_VOID(d) (d->type->primitive == type_void && !decl_ptr_depth(d))

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
				"mismatching %s for arg %d in %s (%s vs. %s)",
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

		one_struct = (!a->decl_ptr && a->type->sue && a->type->sue->primitive != type_enum)
			        || (!b->decl_ptr && b->type->sue && b->type->sue->primitive != type_enum);

		va_start(l, errfmt);
		cc1_warn_atv(w, one_struct || DECL_IS_VOID(a), warn, errfmt, l);
		va_end(l);
	}
}

void fold_typecheck(expr *lhs, expr *rhs, symtable *stab, where *where)
{
	decl *decl_l, *decl_r;

	(void)stab;

	if(!rhs)
		return;

	decl_l = lhs->tree_type;
	decl_r = rhs->tree_type;

	if(DECL_IS_VOID(decl_l) || DECL_IS_VOID(decl_r))
		die_at(where, "use of void expression");


	if(    decl_l->type->primitive == type_enum
			&& decl_r->type->primitive == type_enum
			&& decl_l->type->sue != decl_r->type->sue){

		cc1_warn_at(where, 0, WARN_ENUM_CMP, "expression with enum %s and enum %s",
				decl_l->type->sue->spel, decl_r->type->sue->spel);
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

	const_fold(e);

	old_w = eof_where;
	eof_where = &e->where;
	e->f_fold(e, stab);
	eof_where = old_w;

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());
	UCC_ASSERT(e->tree_type->type->primitive != type_unknown, "unknown type after folding expr %s", e->f_str());
}

void fold_decl_ptr(decl_ptr *dp, symtable *stab, decl *root)
{
	if(dp->fptrargs)
		fold_funcargs(dp->fptrargs, stab, root->spel);

	if(dp->array_size){
		long v;
		fold_expr(dp->array_size, stab);
		if((v = dp->array_size->val.iv.val) < 0)
			die_at(&dp->where, "negative array length");
	}

	if(dp->child)
		fold_decl_ptr(dp->child, stab, root);
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

void fold_decl(decl *d, symtable *stab)
{
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
		if(from->decl_ptr)
			*decl_leaf(d) = decl_ptr_copy(from->decl_ptr);
	}

	UCC_ASSERT(d->type && d->type->store != store_typedef, "typedef store after tdef folding");

	switch(d->type->primitive){
		case type_void:
			if(!decl_ptr_depth(d) && !d->funcargs)
				die_at(&d->where, "can't have a void variable - %s (%s)", d->spel, decl_to_str(d));
			break;

		case type_enum:
		case type_struct:
		case type_union:
			if(sue_incomplete(d->type->sue) && !decl_ptr_depth(d))
				die_at(&d->where, "use of %s%s%s",
						type_to_str(d->type),
						d->spel ?     " " : "",
						d->spel ? d->spel : "");
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
	if(!d->decl_ptr && d->type->qual & qual_restrict)
		die_at(&d->where, "restrict on non-pointer type %s%s%s",
				type_to_str(d->type),
				d->spel ? " " : "",
				d->spel ? d->spel : "");

	if(d->init){
		if(d->type->store == store_extern)
			die_at(&d->where, "error: externs can't be initalised");

		if(d->type->store == store_static || (d->sym && d->sym->type == sym_global)){
			fold_expr(d->init, stab); /* else it's done as part of the stmt code */

			if(!d->init->array_store || d->init->array_store->type != array_exprs){
				char buf_a[DECL_STATIC_BUFSIZ], buf_b[DECL_STATIC_BUFSIZ];

				strcpy(buf_a, decl_to_str(d));
				strcpy(buf_b, decl_to_str(d->init->tree_type));

				fold_decl_equal(d, d->init->tree_type, &d->where, WARN_ASSIGN_MISMATCH,
						"mismatching initialisation for %s (%s vs. %s)",
						d->spel, buf_a, buf_b);
			}
		}

		if(!stab->parent && const_fold(d->init)) /* global + not constant */
			die_at(&d->init->where, "error: not a constant expression (initialiser is %s)", d->init->f_str());
	}

	if(d->funcargs)
		fold_funcargs(d->funcargs, stab, d->spel);

	if(d->decl_ptr)
		fold_decl_ptr(d->decl_ptr, stab, d);

	/*
	 * no need to fold ->init, since these are removed for all but global-decls
	 * (kept in the cast of const-init)
	 */

#undef SPEC
}

void fold_decl_global(decl *d, symtable *stab)
{
	if(d->type->store == store_extern){
		/* we have an extern, check if it's overridden */
		char *const spel = d->spel;
		decl **dit;

		for(dit = stab->decls; dit && *dit; dit++){
			decl *d2 = *dit;
			if(!strcmp(d2->spel, spel) && d2->type->store != store_extern){
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
	decl **diter;

	for(diter = fargs->arglist; diter && *diter; diter++)
		fold_decl(*diter, stab);

	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;

		for(i = 0; fargs->arglist[i]; i++){
			if(type_store_static_or_extern(fargs->arglist[i]->type->store)){
				const char *sp = fargs->arglist[i]->spel;
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
	curdecl_func_sp = func_decl->spel;

	if(func_decl->func_code){
		int nargs, i;

		if(func_decl->funcargs->arglist){
			for(nargs = 0; func_decl->funcargs->arglist[nargs]; nargs++);
			/* add args backwards, since we push them onto the stack backwards - still need to do this here? */
			for(i = nargs - 1; i >= 0; i--){
				if(!func_decl->funcargs->arglist[i]->spel)
					die_at(&func_decl->funcargs->where, "function \"%s\" has unnamed arguments", func_decl->spel);
				else
					SYMTAB_ADD(func_decl->func_code->symtab, func_decl->funcargs->arglist[i], sym_arg);
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
		fargs = df->funcargs = funcargs_new();
		df->spel = ustrdup(ASM_INLINE_FNAME);

		df->type->primitive = type_int;

		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[0] = decl_new();
		fargs->arglist[1] = NULL;
		fargs->arglist[0]->type->primitive = type_char;
		fargs->arglist[0]->type->qual      = qual_const;
		fargs->arglist[0]->decl_ptr        = decl_ptr_new();

		symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);

		eof_where = old_w;
	}

	fold_symtab_scope(globs);

	for(i = 0; D(i); i++)
		if(D(i)->sym)
			ICE("%s: sym (%p) already set for global \"%s\"", where_str(&D(i)->where), D(i)->sym, D(i)->spel);

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
		if(D(i)->funcargs)
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
		char *const spel_i = D(i)->spel;

		if(D(i)->funcargs && !D(i)->func_code){
			int found = 0;
			int j;

			for(j = 0; D(j); j++){
				if(j != i && !strcmp(spel_i, D(j)->spel)){
					/* D(i) is a prototype, check the args match D(j) */
					fold_funcargs_equal(D(i)->funcargs, D(j)->funcargs, 0, &D(j)->where, "type", D(j)->spel);

					if(!decl_equal(D(i), D(j), DECL_CMP_STRICT_PRIMITIVE)){
						char wbuf[WHERE_BUF_SIZ];
						strcpy(wbuf, where_str(&D(j)->where));
						die_at(&D(i)->where, "mismatching return types for function %s (%s)", D(i)->spel, wbuf);
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
				/*cc1_warn_at(&f->where, 0, WARN_EXTERN_ASSUME, "assuming \"%s\" is extern", func_decl->spel);*/
			}
		}
	}

#undef D
}
