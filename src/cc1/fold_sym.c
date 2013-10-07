#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "../util/platform.h"

#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "fold_sym.h"
#include "pack.h"
#include "sue.h"
#include "out/out.h"
#include "fold.h"
#include "fold_sue.h"
#include "decl_init.h"
#include "out/lbl.h"
#include "const.h"
#include "label.h"


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

static void symtab_check_static_asserts(static_assert **sas)
{
	static_assert **i;
	for(i = sas; i && *i; i++){
		static_assert *sa = *i;
		consty k;

		if(sa->checked)
			continue;
		sa->checked = 1;

		FOLD_EXPR(sa->e, sa->scope);
		if(!type_ref_is_integral(sa->e->tree_type))
			die_at(&sa->e->where,
					"static assert: not an integral expression (%s)",
					sa->e->f_str());

		const_fold(sa->e, &k);

		if(k.type != CONST_VAL)
			die_at(&sa->e->where,
					"static assert: not an integer constant expression (%s)",
					sa->e->f_str());

		if(!k.bits.iv.val)
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
	symtable **tabi;

	for(tabi = tab->children; tabi && *tabi; tabi++){
		symtab_check_rw(*tabi);
	}

	for(diter = tab->decls; diter && *diter; diter++){
		decl *const d = *diter;

		if(d->sym) switch(d->sym->type){
			case sym_arg:
        if(!tab->func_exists)
					break;
				/* fall */
			case sym_local:
			{
				/* arg + local checks */
				const int unused = RW_TEST(d, nreads);
				const int has_unused_attr = !!decl_attr_present(d, attr_unused);

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
	return strcmp(IDENT_LOC_SPEL(ia), IDENT_LOC_SPEL(ib));
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

	if(tab->folded)
		return;
	tab->folded = 1;

	for(diter = tab->decls; diter && *diter; diter++){
		decl *d = *diter;

		fold_decl(d, tab, NULL);

		if(d->spel){
			NEW_IDENT(&d->where);
			all_idents[nidents-1].has_decl = 1;
			all_idents[nidents-1].bits.decl = d;
		}

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
					NEW_IDENT(&e->where);
					all_idents[nidents-1].has_decl = 0;
					all_idents[nidents-1].bits.spel = members[i]->enum_member->spel;
				}
			}
		}
	}


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
						/* both enum-membs, fine?????????? or mismatch? */
						clash = "duplicate";
						break;

					case 1:
						/* one enum-memb, one decl */
						clash = "mismatching";
						break;

					default:
					{
						const enum decl_cmp dflags =
							DECL_CMP_EXACT_MATCH | DECL_CMP_ALLOW_TENATIVE_ARRAY;

						decl *da = a->has_decl ? a->bits.decl : NULL;
						decl *db = b->has_decl ? b->bits.decl : NULL;

						const int a_func = da ? !!DECL_IS_FUNC(da) : 0;

						if(!!DECL_IS_FUNC(db) != a_func
						|| !decl_equal(da, db, dflags))
						{
							clash = "mismatching";
						}else{
							if(IS_LOCAL_SCOPE){
								/* allow multiple functions or multiple externs */
								if(a_func){
									/* fine - we know they're equal from decl_equal() above */
								}else if((da->store & STORE_MASK_STORE) == store_extern
										&& (db->store & STORE_MASK_STORE) == store_extern){
									/* both are extern declarations */
								}else{
									clash = "extern/non-extern";
								}
							}else if(a_func && da->func_code && db->func_code){
								clash = "duplicate";
							}
						}
						break;
					}
				}
			}

			if(clash){
				/* XXX: note */
				char wbuf[WHERE_BUF_SIZ];

				die_at(a->w,
						"%s definitions of \"%s\"\n"
						"%s: note: other definition",
						clash, IDENT_LOC_SPEL(a),
						where_str_r(wbuf, b->w));
			}
		}
	}
	free(all_idents);
#undef IS_LOCAL_SCOPE
}

unsigned symtab_layout_decls(symtable *tab, unsigned current)
{
	const unsigned this_start = current;
	int arg_space = 0;

	if(tab->laidout)
		goto out;
	tab->laidout = 1;

	if(tab->decls){
		decl **diter;
		int arg_idx = 0;

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
					s->offset = arg_idx++;
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
							s->offset = current;
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

		/* don't account the args in the space,
		 * just use for offsetting them
		 */
		tab->auto_total_size = current - this_start + subtab_max - arg_space;
	}

out:
	return tab->auto_total_size;
}

void symtab_chk_labels(symtable *stab)
{
	{
		symtable **i;
		for(i = stab->children; i && *i; i++)
			symtab_chk_labels(*i);
	}
	if(stab->labels){
		size_t i;
		label *l;

		for(i = 0;
		    (l = dynmap_value(label *, stab->labels, i));
		    i++)
			if(!l->complete)
				die_at(l->pw, "label '%s' undefined", l->spel);
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

	symtab_check_static_asserts(stab->static_asserts);
}
