#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "fold_sym.h"
#include "../util/platform.h"
#include "../util/dynarray.h"
#include "pack.h"
#include "sue.h"
#include "fold.h"
#include "fold_sue.h"
#include "decl_init.h"
#include "out/lbl.h"
#include "const.h"


#define RW_TEST(var)                                 \
						sym->var == 0                            \
						&& sym->decl->spel                       \
						&& (sym->decl->store & STORE_MASK_STORE) \
						        != store_typedef                 \
						&& !DECL_IS_ARRAY(sym->decl)             \
						&& !DECL_IS_FUNC(sym->decl)              \
						&& !DECL_IS_S_OR_U(sym->decl)

#define RW_SHOW(w, str)                           \
					cc1_warn_at(&sym->decl->where, 0,       \
							WARN_SYM_NEVER_ ## w,               \
							"\"%s\" never " str,                \
							sym->decl->spel);                   \

#define RW_WARN(w, var, str)      \
						do{                   \
							if(RW_TEST(var)){   \
								RW_SHOW(w, str)   \
								sym->var++;       \
							}                   \
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

void symtab_fold_decls(symtable *tab)
{
#define IS_LOCAL_SCOPE !!(tab->parent)
	decl **all_decls = NULL;
	decl **diter;

	if(tab->folded)
		return;
	tab->folded = 1;

	for(diter = tab->decls; diter && *diter; diter++){
		decl *d = *diter;
		sym *const sym = d->sym;
		const int has_unused_attr = !!decl_attr_present(d, attr_unused);

		fold_decl(d, tab, NULL);

		if(d->spel)
			dynarray_add(&all_decls, d);

		if(sym) switch(sym->type){
			case sym_local:
			{
				/* arg + local checks */
				const int unused = RW_TEST(nreads);

				switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
					case store_register:
					case store_default:
					case store_auto:
					case store_static:
						/* static analysis on sym */
						if(!has_unused_attr && !d->init)
							RW_WARN(WRITTEN, nwrites, "written to");
						break;
					case store_extern:
					case store_typedef:
					case store_inline:
						break;
				}
				/* fall */

				if(unused){
					if(!has_unused_attr && (d->store & STORE_MASK_STORE) != store_extern)
						RW_SHOW(READ, "read");
				}else if(has_unused_attr){
					warn_at(&d->where,
							"\"%s\" declared unused, but is used", d->spel);
				}

			case sym_arg:
				/* asm rename checks */
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

			case sym_global:
				break;
		} /* sym switch */
	}


	if(all_decls){
		/* check_clashes */
		decl **di;

		qsort(all_decls,
				dynarray_count(all_decls),
				sizeof *all_decls,
				(int (*)(const void *, const void *))decl_sort_cmp);

		for(di = all_decls; di[1]; di++){
			decl *a = di[0], *b = di[1];
			char *clash = NULL;

			/* we allow multiple function declarations,
			 * and multiple declarations at global scope,
			 * but not definitions
			 */
			if(!strcmp(a->spel, b->spel)){
				const int a_func = !!DECL_IS_FUNC(a);

				if(!!DECL_IS_FUNC(b) != a_func){
					clash = "mismatching";
				}else switch(decl_cmp(a, b, TYPE_CMP_ALLOW_TENATIVE_ARRAY)){
					case TYPE_NOT_EQUAL:
						/* must be an exact match */
					case TYPE_QUAL_LOSS:
					case TYPE_CONVERTIBLE_IMPLICIT:
					case TYPE_CONVERTIBLE_EXPLICIT:
						clash = "mismatching";
					case TYPE_EQUAL:
						break;
				}

				if(!clash){
					if(IS_LOCAL_SCOPE){
						/* allow multiple functions or multiple externs */
						if(a_func){
							/* fine - we know they're equal from decl_equal() above */
						}else if((a->store & STORE_MASK_STORE) == store_extern
						      && (b->store & STORE_MASK_STORE) == store_extern){
							/* both are extern declarations */
						}else{
							clash = "extern/non-extern";
						}
					}else if(a_func && a->func_code && b->func_code){
						clash = "duplicate";
					}
				}
			}

			if(clash){
				/* XXX: note */
				char wbuf[WHERE_BUF_SIZ];

				die_at(&a->where,
						"%s definitions of \"%s\"\n"
						"%s: note: other definition",
						clash, a->spel,
						where_str_r(wbuf, &b->where));
			}
		}


		dynarray_free(decl **, &all_decls, NULL);
	}
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

void symtab_fold_sues(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++)
		fold_sue(*sit, stab);

	symtab_check_static_asserts(stab->static_asserts);
}

void symtab_fold_decls_sues(symtable *stab)
{
	symtab_fold_sues(stab);
	symtab_fold_decls(stab);
}
