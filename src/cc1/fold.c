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
#include "struct.h"
#include "enum.h"
#include "struct_enum.h"

#define EXPR_NON_VOID(e, s) \
	if(!decl_ptr_depth(e->tree_type) && e->tree_type->type->primitive == type_void) \
		die_at(&e->where, "%s requires non-void expression", s)

char *curdecl_func_sp;           /* for funcargs-local labels */
static tree *curtree_switch;     /* for case + default */
static tree *curtree_flow;       /* for break */


void fold_funcargs_equal(funcargs *args_a, funcargs *args_b, int check_vari, where *w, const char *warn_pre, const char *func_spel)
{
	const int count_a = dynarray_count((void **)args_a->arglist);
	const int count_b = dynarray_count((void **)args_b->arglist);
	int i;

	if(!(check_vari && args_a->variadic ? count_a <= count_b : count_a == count_b)){
		char wbuf[WHERE_BUF_SIZ];
		strcpy(wbuf, where_str(&args_a->where));
		die_at(w, "mismatching argument counts for function %s (%s)", func_spel, wbuf);
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
		char buf[DECL_STATIC_BUFSIZ];
		va_list l;

		strcpy(buf, decl_to_str(b));

		cc1_warn_at(w, 0, warn, "%s vs. %s for...", decl_to_str(a), buf);

		va_start(l, errfmt);
		cc1_warn_atv(w, a->type->primitive == type_void && !decl_ptr_depth(a), warn, errfmt, l);
		va_end(l);
	}
}

void fold_expr(expr *e, symtable *stab)
{
	if(e->spel && !e->sym)
		e->sym = symtab_search(stab, e->spel);

	const_fold(e);

	e->f_fold(e, stab);

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

void fold_decl(decl *d, symtable *stab)
{
	switch(d->type->primitive){
		int incomplete;

		case type_void:
			if(!decl_ptr_depth(d) && !d->funcargs)
				die_at(&d->type->where, "can't have a void variable (%s)", decl_to_str(d));
			break;

		case type_enum:
			st_en_lookup((void **)&d->type->enu,   &incomplete, d, stab, (void *(*)(struct symtable *, const char *))enum_find,   0);
			goto incomp_check;

		case type_struct:
			st_en_lookup((void **)&d->type->struc, &incomplete, d, stab, (void *(*)(struct symtable *, const char *))struct_find, 1);
incomp_check:
			if(incomplete && !decl_ptr_depth(d))
				die_at(&d->where, "use of incomplete type \"%s\"", d->spel);
			break;

		default:
			break;
	}

	if(d->funcargs)
		fold_funcargs(d->funcargs, stab, d->spel);

#define SPEC(x) (d->type->spec & (x))

	/* type spec checks */
	if(SPEC(spec_extern) && SPEC(spec_static))
		die_at(&d->type->where, "can't have static extern");

	if(SPEC(spec_signed) && SPEC(spec_unsigned))
		die_at(&d->type->where, "can't have signed unsigned");

	if(SPEC(spec_auto)){
#define CANT_HAVE(sp) \
		if(SPEC(sp)) \
			die_at(&d->type->where, "can't have auto %s", spec_to_str(sp))

		CANT_HAVE(spec_static);
		CANT_HAVE(spec_extern);

#undef CANT_HAVE
	}

	if(d->decl_ptr)
		fold_decl_ptr(d->decl_ptr, stab, d);

#undef SPEC
}

void fold_decl_global(decl *d, symtable *stab)
{
	if(d->init){
		if(d->type->spec & spec_extern)
			/* only need this check for globals, since block-decls aren't initalised */
			die_at(&d->where, "externs can't be initalised");

		fold_expr(d->init, stab);

		if(const_fold(d->init))
			die_at(&d->init->where, "not a constant expression (initialiser is %s)", d->init->f_str());

	}else if(d->type->spec & spec_extern){
		/* we have an extern, check if it's overridden */
		char *const spel = d->spel;
		decl **dit;

		for(dit = stab->decls; dit && *dit; dit++){
			decl *d2 = *dit;
			if(!strcmp(d2->spel, spel) && (d2->type->spec & spec_extern) == 0){
				/* found an override */
				d->ignore = 1;
				break;
			}
		}
	}

	fold_decl(d, stab);
}

int fold_struct(struct_st *st)
{
	int offset;
	decl **i;

	for(offset = 0, i = st->members; i && *i; i++){
		decl *d = *i;
		if(d->type->primitive == type_struct && !d->type->struc)
			ICE("TODO: struct lookup");

		if(d->type->struc){
			d->struct_offset = offset;
			offset += fold_struct(d->type->struc);
		}else{
			d->struct_offset = offset;
			offset += decl_size(d);
		}
	}

	return offset;
}

void fold_enum(enum_st *en, symtable *stab)
{
	enum_member **i;
	int defval = 0;

	for(i = en->members; *i; i++){
		enum_member *m = *i;
		expr *e = m->val;

		if(e == (expr *)-1){
			/*expr_free(e); XXX: memleak */
			m->val = expr_new_val(defval++);
		}else{
			fold_expr(e, stab);
			if(!const_expr_is_const(e))
				die_at(&e->where, "enum value not constant");
			defval = const_expr_val(e) + 1;
		}
	}
}

void fold_symtab_scope(symtable *stab)
{
	struct_st **sit;
	enum_st   **eit;

	/* fold structs, then enums, then decls - decls may rely on enums */
	for(sit = stab->structs; sit && *sit; sit++)
		fold_struct(*sit);

	for(eit = stab->enums; eit && *eit; eit++)
		fold_enum(*eit, stab);
}

void fold_switch_enum(tree *sw, type *enum_type)
{
	const int nents = enum_nentries(enum_type->enu);
	tree **titer;
	char *marks = umalloc(nents * sizeof *marks);
	int midx;

	/* for each case/default/case_range... */
	for(titer = sw->codes; titer && *titer; titer++){
		tree *cse = *titer;
		int v, w;

		if(cse->expr->expr_is_default)
			goto ret;

		v = cse->expr->val.iv.val;

		if(cse->type == stat_case_range)
			w = cse->expr2->val.iv.val;
		else
			w = v;

		for(; v <= w; v++){
			enum_member **mi;
			for(midx = 0, mi = enum_type->enu->members; *mi; midx++, mi++)
				if(v == (*mi)->val->val.iv.val)
					marks[midx]++;
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, 0, WARN_SWITCH_ENUM, "enum %s not handled in switch", enum_type->enu->members[midx]->spel);

ret:
	free(marks);
}

void fold_tree(tree *t)
{
	UCC_ASSERT(t->symtab->parent, "symtab has no parent");

	switch(t->type){
		case stat_break:
			if(!curtree_flow)
				die_at(&t->expr->where, "break outside a flow-control statement");

			t->expr = expr_new_identifier(curtree_flow->lblfin);
			t->expr->tree_type = decl_new();
			t->expr->tree_type->type->primitive = type_int;
			break;

		case stat_goto:
		case stat_label:
		{
			char *save = t->expr->spel;
			if(!expr_kind(t->expr, identifier))
				die_at(&t->expr->where, "not a label identifier");
			/* else let the assembler check for link errors */
			t->expr->spel = asm_label_goto(t->expr->spel);
			free(save);

			if(t->type == stat_label)
				fold_tree(t->lhs); /* compound */
			break;
		}

		case stat_while:
		case stat_do:
		{
			tree *oldflowtree = curtree_flow;
			curtree_flow = t;

			t->lblfin = asm_label_flowfin();

		case stat_if:
			fold_expr(t->expr, t->symtab);
			EXPR_NON_VOID(t->expr, stat_to_str(t->type));

			OPT_CHECK(t->expr, "constant expression in if/while");

			fold_tree(t->lhs);

			if(t->type != stat_if)
				curtree_flow = oldflowtree;
			else if(t->rhs)
				fold_tree(t->rhs);
			break;
		}

		case stat_for:
		{
			tree *oldflowtree = curtree_flow;
			curtree_flow = t;

			t->lblfin = asm_label_flowfin();

#define FOLD_IF(x) if(x) fold_expr(x, t->symtab)
			FOLD_IF(t->flow->for_init);
			FOLD_IF(t->flow->for_while);
			FOLD_IF(t->flow->for_inc);
#undef FOLD_IF

			if(t->flow->for_while)
				EXPR_NON_VOID(t->flow->for_while, "for-while");

			OPT_CHECK(t->flow->for_while, "constant expression in for");

			fold_tree(t->lhs);

			curtree_flow = oldflowtree;
			break;
		}


		case stat_case_range:
		{
			int l, r;

			fold_expr(t->expr,  t->symtab);
			fold_expr(t->expr2, t->symtab);

			if(const_fold(t->expr) || const_fold(t->expr2))
				die_at(&t->where, "case range not constant");

			EXPR_NON_VOID(t->expr,  "case");
			EXPR_NON_VOID(t->expr2, "case");

			l = t->expr->val.iv.val;
			r = t->expr2->val.iv.val;

			if(l >= r)
				die_at(&t->where, "case range equal or inverse");

			t->expr->spel = asm_label_case(CASE_RANGE, l);
			goto case_add;
		}

		case stat_case:
			fold_expr(t->expr, t->symtab);

			EXPR_NON_VOID(t->expr, "case");

			if(const_fold(t->expr))
				die_at(&t->expr->where, "case expression not constant");
			/* fall */
		case stat_default:
			if(t->expr){
				t->expr->spel = asm_label_case(CASE_CASE, t->expr->val.iv.val);
			}else{
				t->expr = expr_new_identifier(NULL);
				t->expr->spel = asm_label_case(CASE_CASE, t->expr->val.iv.val);
				t->expr->expr_is_default = 1;
			}
case_add:
			fold_tree(t->lhs); /* compound */
			if(!curtree_switch)
				die_at(&t->expr->where, "not inside a switch statement");
			dynarray_add((void ***)&curtree_switch->codes, t);
			break;

		case stat_switch:
		{
			tree *oldswtree = curtree_switch;
			tree *oldflowtree = curtree_flow;
			type *typ;

			curtree_switch = t;
			curtree_flow   = t;

			t->lblfin = asm_label_flowfin();

			fold_expr(t->expr, t->symtab);

			EXPR_NON_VOID(t->expr, "switch");

			OPT_CHECK(t->expr, "constant expression in switch");

			fold_tree(t->lhs);
			/* FIXME: check for duplicate case values and at most, 1 default */

			/* check for an enum */
			typ = t->expr->tree_type->type;
			if(typ->primitive == type_enum){
				UCC_ASSERT(typ->enu, "no enum for enum type");
				fold_switch_enum(t, typ);
			}

			curtree_switch = oldswtree;
			curtree_flow   = oldflowtree;
			break;
		}

		case stat_code:
		{
			decl **iter;

			fold_symtab_scope(t->symtab);

			for(iter = t->decls; iter && *iter; iter++){
				decl *d = *iter;

				if(d->func_code)
					die_at(&d->func_code->where, "can't nest functions");

				fold_decl(d, t->symtab);

				SYMTAB_ADD(t->symtab, d, sym_local);
			}

			if(t->codes){
				tree **iter;
				for(iter = t->codes; *iter; iter++)
					fold_tree(*iter);
			}

			/* static folding */
			if(t->decls){
				decl **iter;

				for(iter = t->decls; *iter; iter++){
					decl *d = *iter;
					/*
					 * check static decls - after we fold,
					 * so we've linked the syms and can change ->spel
					 */
					if(d->type->spec & spec_static){
						char *save = d->spel;
						d->spel = asm_label_static_local(curdecl_func_sp, d->spel);
						free(save);
					}
				}
			}
			break;
		}

		case stat_return:
			if(t->expr){
				fold_expr(t->expr, t->symtab);
				EXPR_NON_VOID(t->expr, "return");
			}
			break;

		case stat_expr:
			fold_expr(t->expr, t->symtab);
			break;

		case stat_noop:
			break;
	}
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
			if(fargs->arglist[i]->type->spec & (spec_static | spec_extern)){
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

		fold_tree(func_decl->func_code);

		curdecl_func_sp = NULL;
	}
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	int i;

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *df;
		funcargs *fargs;

		df = decl_new();
		fargs = df->funcargs = funcargs_new();
		df->spel = ustrdup(ASM_INLINE_FNAME);

		df->type->primitive = type_int;

		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[0] = decl_new();
		fargs->arglist[1] = NULL;
		fargs->arglist[0]->type->primitive = type_char;
		fargs->arglist[0]->type->spec     |= spec_const;
		fargs->arglist[0]->decl_ptr        = decl_ptr_new();

		symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);
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
				D(i)->type->spec |= spec_extern;
				/*cc1_warn_at(&f->where, 0, WARN_EXTERN_ASSUME, "assuming \"%s\" is extern", func_decl->spel);*/
			}
		}
	}

#undef D
}
