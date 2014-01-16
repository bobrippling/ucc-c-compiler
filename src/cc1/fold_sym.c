#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "../util/platform.h"

#include "cc1.h"
#include "sym.h"
#include "fold_sym.h"
#include "pack.h"
#include "sue.h"
#include "fold.h"
#include "fold_sue.h"
#include "decl_init.h"
#include "out/lbl.h"
#include "const.h"
#include "label.h"
#include "type_is.h"


#define RW_TEST(decl, var)                      \
            decl->sym->var == 0                 \
            && decl->spel                       \
            && (decl->store & STORE_MASK_STORE) \
                    != store_typedef            \
            && !DECL_IS_ARRAY(decl)             \
            && !DECL_IS_FUNC(decl)              \
            && !DECL_IS_S_OR_U(decl)

#define RW_SHOW(decl, w, str)          \
          cc1_warn_at(&decl->where, 0, \
              WARN_SYM_NEVER_ ## w,    \
              "\"%s\" never " str,     \
              decl->spel);             \

#define RW_WARN(w, decl, var, str)    \
            do{                       \
              if(RW_TEST(decl, var)){ \
                RW_SHOW(decl, w, str) \
                decl->sym->var++;     \
              }                       \
            }while(0)

/*#define SYMTAB_DEBUG*/
#ifdef SYMTAB_DEBUG
static void print_stab(symtable *st, int indent)
{
#define STAB_INDENT() for(i = 0; i < indent; i++) fputs("  ", stderr)
	int i;

	STAB_INDENT();

	fprintf(stderr, "table %p, children %d, vars %d, are_params %d, parent: %p\n",
			(void *)st,
			dynarray_count(st->children),
			dynarray_count(st->decls),
			st->are_params,
			(void *)st->parent);

	decl **di;
	for(di = st->decls; di && *di; di++){
		decl *d = *di;
		STAB_INDENT();
		fprintf(stderr, "  (%s, %s)\n",
				d->sym ? sym_to_str(d->sym->type) : NULL,
				decl_to_str(d));
	}

	symtable **si;
	for(si = st->children; si && *si; si++)
		print_stab(*si, indent + 1);
}
#endif

static void symtab_iter_children(symtable *stab, void f(symtable *))
{
	symtable **i;

	for(i = stab->children; i && *i; i++)
		f(*i);
}

void symtab_check_static_asserts(symtable *stab)
{
	static_assert **i;

	symtab_iter_children(stab, symtab_check_static_asserts);

	for(i = stab->static_asserts; i && *i; i++){
		static_assert *sa = *i;
		consty k;

		if(sa->checked)
			continue;
		sa->checked = 1;

		FOLD_EXPR(sa->e, sa->scope);
		if(!type_is_integral(sa->e->tree_type))
			die_at(&sa->e->where,
					"static assert: not an integral expression (%s)",
					sa->e->f_str());

		const_fold(sa->e, &k);

		if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num))
			die_at(&sa->e->where,
					"static assert: not an integer constant expression (%s)",
					sa->e->f_str());

		if(!k.bits.num.val.i)
			die_at(&sa->e->where, "static assertion failure: %s", sa->s);

		if(fopt_mode & FOPT_SHOW_STATIC_ASSERTS){
			fprintf(stderr, "%s: static assert passed: %s-expr, msg: %s\n",
					where_str(&sa->e->where), sa->e->f_str(), sa->s);
		}
	}
}

void symtab_check_rw(symtable *tab)
{
	decl **diter;

	symtab_iter_children(tab, symtab_check_rw);

	for(diter = tab->decls; diter && *diter; diter++){
		decl *const d = *diter;

		if(d->sym) switch(d->sym->type){
			case sym_arg:
				if(!tab->in_func)
					break;
				/* fall */
			case sym_local:
			{
				/* arg + local checks */
				const int unused = RW_TEST(d, nreads);
				const int has_unused_attr = !!attribute_present(d, attr_unused);

				if(d->sym->type != sym_arg){
					switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
						case store_register:
						case store_default:
						case store_auto:
						case store_static:
							/* static analysis on sym */
							if(!has_unused_attr && !d->init)
								RW_WARN(WRITTEN, d, nwrites, "written to");
							break;
						case store_extern:
						case store_typedef:
						case store_inline:
							break;
					}
				}

				if(unused){
					if(!has_unused_attr && (d->store & STORE_MASK_STORE) != store_extern)
						RW_SHOW(d, READ, "read");
				}else if(has_unused_attr){
					warn_at(&d->where,
							"\"%s\" declared unused, but is used", d->spel);
				}
			}

			case sym_global:
				break;
		} /* sym switch */
	}
}

struct ident_loc
{
	where *w;
	int has_decl;
	union
	{
		decl *decl;
		const char *spel;
	} bits;
};
#define IDENT_LOC_SPEL(il) \
	((il)->has_decl          \
	 ? (il)->bits.decl->spel \
	 : (il)->bits.spel)

static int ident_loc_cmp(const void *a, const void *b)
{
	const struct ident_loc *ia = a, *ib = b;
	int r = strcmp(IDENT_LOC_SPEL(ia), IDENT_LOC_SPEL(ib));

	/* sort according to spel, then according to func_code
	 * so it makes checking redefinitions easier, e.g.
	 * f(){} f(); f(){}
	 */
	if(r == 0 && ia->has_decl && ib->has_decl)
		r = !!ia->bits.decl->func_code - !!ib->bits.decl->func_code;

	return r;
}

static void warn_c11_retypedef(decl *a, decl *b)
{
	/* repeated typedefs are allowed in C11 - 6.7.3 */
	if(cc1_std < STD_C11){
		char buf[WHERE_BUF_SIZ];

		warn_at(&b->where,
				"typedef '%s' redefinition is a C11 extension\n"
				"%s: note: other definition here",
				a->spel, where_str_r(buf, &a->where));
	}
}

void symtab_fold_decls(symtable *tab)
{
#define IS_LOCAL_SCOPE !!(tab->parent)
	decl **diter;

	struct ident_loc *all_idents = NULL;
	size_t nidents = 0;
#define NEW_IDENT(pw) do{                      \
		  all_idents = urealloc1(                  \
		      all_idents,                          \
		      (nidents + 1) * sizeof *all_idents); \
		  all_idents[nidents].w = pw;              \
		  nidents++;                               \
		}while(0)

#define NEW_DECL(d) do{                     \
		  NEW_IDENT(&(d)->where);               \
		  all_idents[nidents-1].has_decl = 1;   \
		  all_idents[nidents-1].bits.decl = d;  \
		}while(0)

#ifdef SYMTAB_DEBUG
	if(!tab->parent)
		print_stab(tab, 0);
#endif

	symtab_iter_children(tab, symtab_fold_decls);

	if(tab->folded)
		return;
	tab->folded = 1;

	for(diter = tab->decls; diter && *diter; diter++){
		decl *d = *diter;

		fold_decl(d, tab, NULL);

		if(d->spel)
			NEW_DECL(d);

		/* asm rename checks */
		if(d->sym && d->sym->type != sym_global){
			switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
				case store_register:
				case store_extern:
				case store_static:
					break;
				default:
					/* allow anonymous decls to have .spel_asm */
					if(d->spel && d->spel_asm){
						die_at(&d->where,
								"asm() rename on non-register non-global variable \"%s\" (%s)",
								d->spel, d->spel_asm);
					}
			}
		}
	}

	/* add enums */
	{
		struct_union_enum_st **ei;
		for(ei = tab->sues; ei && *ei; ei++){
			struct_union_enum_st *e = *ei;

			if(e->primitive == type_enum){
				sue_member **const members = e->members;
				int i;

				for(i = 0; members && members[i]; i++){
					enum_member *emem = members[i]->enum_member;
					NEW_IDENT(&emem->where);
					all_idents[nidents-1].has_decl = 0;
					all_idents[nidents-1].bits.spel = emem->spel;
				}
			}
		}
	}

	/* add args */
	if(tab->parent && tab->parent->are_params)
		for(diter = tab->parent->decls; diter && *diter; diter++)
			NEW_DECL(*diter);

	if(nidents > 1){
		/* check_clashes */
		size_t i;

		qsort(all_idents, nidents,
				sizeof *all_idents,
				&ident_loc_cmp);

		for(i = 0; i < nidents - 1; i++){
			struct ident_loc *a = all_idents + i,
			                 *b = all_idents + i + 1;
			char *clash = NULL;

			/* we allow multiple function declarations,
			 * and multiple declarations at global scope,
			 * but not definitions
			 */
			if(!strcmp(IDENT_LOC_SPEL(a), IDENT_LOC_SPEL(b))){
				switch(a->has_decl + b->has_decl){
					case 0:
						/* both enum-membs, mismatch */
						clash = "duplicate";
						break;

					case 1:
						/* one enum-memb, one decl */
						clash = "mismatching";
						break;

					default:
					{
						/* non-null based on switch */
						UCC_ASSERT(a->has_decl && b->has_decl, "no decls?");
					}
					{
						decl *da = a->bits.decl;
						decl *db = b->bits.decl;

						const int a_func = !!DECL_IS_FUNC(da);

						if(!!DECL_IS_FUNC(db) != a_func){
							clash = "mismatching";
						}else switch(decl_cmp(da, db, TYPE_CMP_ALLOW_TENATIVE_ARRAY)){
							case TYPE_NOT_EQUAL:
							case TYPE_QUAL_CHANGE:
								/* must be an exact match */
							case TYPE_QUAL_LOSS:
							case TYPE_CONVERTIBLE_IMPLICIT:
							case TYPE_CONVERTIBLE_EXPLICIT:
								/* allow static/non-static redecl for non-functions */
								if(a_func || (da->store & STORE_MASK_STORE) != store_static)
									clash = "mismatching";
								break;

							case TYPE_EQUAL_TYPEDEF:
							case TYPE_EQUAL:
								if(IS_LOCAL_SCOPE){
									enum decl_storage a_store = da->store & STORE_MASK_STORE;
									enum decl_storage b_store = db->store & STORE_MASK_STORE;
									int a_extern = a_store == store_extern;
									int b_extern = b_store == store_extern;

									/* local scope - any decl without linkage can be
									 * specified once and once only - C99 6.7.3
									 */
									if(a_extern != b_extern){
										clash = "extern/non-extern";
									}else if(a_extern + b_extern == 0){
										/* redefinition at local scope - allow typedef */
										if(a_store & store_typedef && b_store & store_typedef){
											warn_c11_retypedef(da, db);
										}else if(!a_func){ /* functions can repeat */
											clash = "duplicate";
										}
									}else{
										/* fine - both extern */
									}
								}else if(a_func && da->func_code && db->func_code){
									clash = "duplicate";
								}else if((da->store & STORE_MASK_STORE) == store_typedef){
									warn_c11_retypedef(da, db);
								}
								break;
						}
						break;
					}
				}
			}

			if(clash){
				/* XXX: note */
				char wbuf[WHERE_BUF_SIZ];

				die_at(b->w,
						"%s definitions of \"%s\"\n"
						"%s: note: previous definition",
						clash, IDENT_LOC_SPEL(a),
						where_str_r(wbuf, a->w));
			}
		}
	}
	free(all_idents);
#undef IS_LOCAL_SCOPE
}

unsigned symtab_layout_decls(symtable *tab, unsigned current)
{
	const unsigned this_start = current;

	if(tab->laidout)
		goto out;
	tab->laidout = 1;

	if(tab->decls){
		decl **diter;

		for(diter = tab->decls; *diter; diter++){
			decl *d = *diter;
			sym *s = d->sym;

			/* we might not have a symbol, e.g.
			 * f(int (*pf)(int (*callme)()))
			 *         ^         ^
			 *         |         +-- nested - skipped
			 *         +------------ `tab'
			 */
			if(!s)
				continue;


			switch(s->type){
				case sym_arg:
					break;

				case sym_local: /* warn on unused args and locals */
					if(DECL_IS_FUNC(d))
						continue;

					switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
							/* for now, we allocate stack space for register vars */
						case store_register:
						case store_default:
						case store_auto:
						{
							unsigned siz = decl_size(s->decl);
							unsigned align = decl_align(s->decl);

							/* align greater than size - we increase
							 * size so it can be aligned to `align'
							 */
							if(align > siz)
								siz = pack_to_align(siz, align);

							/* packing takes care of everything */
							pack_next(&current, NULL, siz, align);
							s->loc.stack_pos = current;
							break;
						}

						case store_static:
						case store_extern:
						case store_typedef:
							break;
						case store_inline:
							ICE("%s store", decl_store_to_str(d->store));
					}
				case sym_global:
					break;
			}
		}
	}

	{
		symtable **tabi;
		unsigned subtab_max = 0;

		for(tabi = tab->children; tabi && *tabi; tabi++){
			unsigned this = symtab_layout_decls(*tabi, current);
			if(this > subtab_max)
				subtab_max = this;
		}

		/* don't account the args in the space */
		tab->auto_total_size = current - this_start + subtab_max;
	}

out:
	return tab->auto_total_size;
}

void symtab_chk_labels(symtable *stab)
{
	symtab_iter_children(stab, symtab_chk_labels);

	if(stab->labels){
		size_t i;
		label *l;

		for(i = 0;
		    (l = dynmap_value(label *, stab->labels, i));
		    i++)
			if(!l->complete)
				die_at(l->pw, "label '%s' undefined", l->spel);
			else if(!l->uses && !l->unused)
				warn_at(l->pw, "unused label '%s'", l->spel);
	}
}

void symtab_fold_sues(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++)
		fold_sue(*sit, stab);
}

void symtab_fold_decls_sues(symtable *stab)
{
	symtab_fold_sues(stab);
	symtab_fold_decls(stab);
}
