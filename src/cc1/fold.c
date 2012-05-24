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
#include "../util/dynmap.h"
#include "sue.h"
#include "decl.h"

char *curdecl_func_sp;       /* for funcargs-local labels */
stmt *curstmt_flow;          /* for break + continue */
stmt *curstmt_switch;        /* for case + default */
int   curstmt_last_was_switch;

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

void fold_coerce_assign(decl *d, expr *assign, int *ok)
{
	/*
	 * assignment coercion - complete incomplete arrays
	 * and convert type[] to struct inits for structs
	 */
	*ok = 0;

	if(!decl_ptr_depth(d) && d->type->primitive == type_struct){
		if(assign->array_store){
			array_decl *store = assign->array_store;

			if(store->struct_idents){
				int i;

				for(i = 0; store->struct_idents[i]; i++){
					fprintf(stderr, ".%s = %s\n",
							store->struct_idents[i],
							decl_to_str(store->data.exprs[i]->tree_type));
				}

				ICE("TODO: struct init from ^");
			}else{
				decl_desc *dp = decl_array_first(assign->tree_type);
				int narray, nmembers;

				*ok = 1;

				/*
				* for now just check the counts - this will break for:
				* struct { int i; char c; int j } = { 1, 2, 3 };
				*                   ^
				* in global scope
				*/
				narray = dp->bits.array_size->val.iv.val;
				nmembers = sue_nmembers(d->type->sue);

				if(narray != nmembers){
					warn_at(&assign->where,
							"mismatching member counts for struct init (struct of %d vs array of %d)",
							nmembers, narray);
					/* TODO: zero the rest */
				}else if(!d->sym || d->sym->type == sym_global){
					sue_member **i;

					for(i = d->type->sue->members; i && *i; i++){
						decl *d = &(*i)->struct_member;
						if(!decl_ptr_depth(d) && d->type->primitive == type_char){
							warn_at(&assign->where, "struct init via { } breaks with char member (%s %s)",
									decl_to_str(d), d->spel);
							break;
						}
					}
				}
			}
		}else{
			ICE("struct init from %s", decl_to_str(assign->tree_type));
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
				(void *)d->type->typeof->decl,
				(void *)d->type->typeof->expr->tree_type);

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

	if(d->field_width && !decl_is_integral(d))
		die_at(&d->where, "field width on non-integral type %s", decl_to_str(d));


	if(decl_is_func(d)){
		switch(d->type->store){
			case store_register:
			case store_auto:
				die_at(&d->where, "%s storage for function", type_store_to_str(d->type->store));
			default:
				break;
		}
	}else{
		if(d->type->is_inline)
			warn_at(&d->where, "inline on non-function%s%s",
					d->spel ? " " : "",
					d->spel ? d->spel : "");
	}

	if(d->init){
		if(d->type->store == store_extern){
			/* allow for globals - remove extern since it's a definition */
			if(stab->parent){
				die_at(&d->where, "externs can't be initialised");
			}else{
				warn_at(&d->where, "extern initialisation");
				d->type->store = store_default;
			}
		}

		if(decl_has_incomplete_array(d) && d->init->array_store){
			/* complete the decl */
			decl_desc *dp = decl_array_first_incomplete(d);

			dp->bits.array_size->val.iv.val = d->init->array_store->len;
		}

		/* type check for statics + globals */
		if(d->type->store == store_static || (d->sym && d->sym->type == sym_global)){
			int ok;

			fold_expr(d->init, stab); /* else it's done as part of the stmt code */
			fold_coerce_assign(d, d->init, &ok); /* also done as stmt code */

			if(!ok){
				char buf_a[DECL_STATIC_BUFSIZ], buf_b[DECL_STATIC_BUFSIZ];
				strcpy(buf_a, decl_to_str(d));
				strcpy(buf_b, decl_to_str(d->init->tree_type));

				fold_decl_equal(d, d->init->tree_type, &d->where, WARN_ASSIGN_MISMATCH,
						"mismatching initialisation for %s (%s vs. %s)",
						decl_spel(d), buf_a, buf_b);
			}

			if(const_fold(d->init) && !const_expr_is_const(d->init)){
				/* global/static + not constant */
				/* allow identifiers if the identifier is also static */

				if(!expr_kind(d->init, identifier) || d->init->tree_type->type->store != store_static){
					die_at(&d->init->where, "not a constant expression for %s init - %s", d->spel, d->init->f_str());
				}
			}
		}
	}
}

void fold_decl_global(decl *d, symtable *stab)
{
	switch(d->type->store){
		case store_extern:
		case store_default:
		case store_static:
			break;

		case store_typedef:
			ICE("typedef store");

		case store_auto:
		case store_register:
			die_at(&d->where, "invalid storage class %s on global scoped %s",
					type_store_to_str(d->type->store),
					decl_is_func(d) ? "function" : "variable");
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

static void fold_link_decl_defs(decl **decls)
{
	dynmap *spel_decls = dynmap_new((dynmap_cmp_f *)strcmp);
	decl **diter;
	int i;

	for(diter = decls; diter && *diter; diter++){
		decl *const d   = *diter;
		char *const key = d->spel;
		decl **val;

		val = dynmap_get(spel_decls, key);

		dynarray_add((void ***)&val, d); /* fine if val is null */

		dynmap_set(spel_decls, key, val);
	}

	for(i = 0; ; i++){
		char *key;
		char wbuf[WHERE_BUF_SIZ];
		decl *d, *e, *definition, *first_none_extern;
		decl **decls_for_this, **decl_iter;

		key = dynmap_key(spel_decls, i);
		if(!key)
			break;

		decls_for_this = dynmap_get(spel_decls, key);
		d = *decls_for_this;

		definition = decl_is_definition(d) ? d : NULL;
		first_none_extern = d->type->store != store_extern ? d : NULL;

		/*
		 * check the first is equal to all the rest, strict-types
		 * check they all have the same static/non-static storage
		 * if all are extern (and not initialised), the decl is extern
		 * if all are extern but there is an init, the decl is global
		 */

		for(decl_iter = decls_for_this + 1; (e = *decl_iter); decl_iter++){
			/* check they are the same decl */
			if(!decl_equal(d, e, DECL_CMP_STRICT_PRIMITIVE)){
				strcpy(wbuf, where_str(&d->where));
				die_at(&e->where, "mismatching declaration of %s (%s)", d->spel, wbuf);
			}

			if( d->type->store != e->type->store
			&& (d->type->store == store_static || e->type->store == store_static))
			{
				strcpy(wbuf, where_str(&d->where));
				die_at(&e->where, "static/non-static mismatch of %s (%s)", d->spel, wbuf);
			}

			if(decl_is_definition(e)){
				/* e is the implementation/instantiation */

				if(definition){
					/* already got one */
					strcpy(wbuf, where_str(&d->where));
					die_at(&e->where, "duplicate definition of %s (%s)", d->spel, wbuf);
				}

				definition = e;
			}

			if(!first_none_extern && e->type->store != store_extern)
				first_none_extern = e;
		}

		if(!definition){
      /* implicit definition - attempt none extern if we have one */
      if(first_none_extern)
        definition = first_none_extern;
      else
        definition = d;
		}

		definition->is_definition = 1;
	}

	dynmap_free(spel_decls);
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
			ICE("%s: sym (%p) already set for global \"%s\"", where_str(&D(i)->where), (void *)D(i)->sym, decl_spel(D(i)));

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

	/* link declarations with definitions */
	fold_link_decl_defs(globs->decls);

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
