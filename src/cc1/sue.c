#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "../util/dynarray.h"

#include "num.h"
#include "sue.h"
#include "cc1.h"
#include "cc1_where.h"
#include "expr.h"
#include "decl.h"
#include "type_is.h"
#include "fopt.h"
#include "parse_fold_error.h"
#include "fold.h"

static void set_spel(struct_union_enum_st *sue, char *spel)
{
	if(!spel){
		int len = 6 + 6 + 3 + WHERE_BUF_SIZ + 1 + 1;
		spel = umalloc(len);
		snprintf(spel, len, "<anon %s @ %s>",
				sue_str(sue), where_str(&sue->where));
	}

	assert(!sue->spel);
	sue->spel = spel;
}

static struct_union_enum_st *sue_new(
		enum type_primitive prim,
		const where *loc,
		char *spel)
{
	struct_union_enum_st *sue = umalloc(sizeof *sue);

	sue->primitive = prim;
	sue->foldprog = SUE_FOLDED_NO;
	memcpy_safe(&sue->where, loc);

	sue->anon = !spel;
	set_spel(sue, spel);

	return sue;
}

void enum_vals_add(
		sue_member ***pmembers,
		where *w,
		char *sp, expr *e,
		attribute **attr)
{
	enum_member *emem = umalloc(sizeof *emem);
	sue_member *mem = umalloc(sizeof *mem);

	if(!e)
		e = (expr *)-1;

	emem->spel = sp;
	emem->val  = e;
	dynarray_add_tmparray(&emem->attr, attr);
	memcpy_safe(&emem->where, w);

	mem->enum_member = emem;

	dynarray_add(pmembers, mem);
}

int enum_nentries(struct_union_enum_st *e)
{
	return dynarray_count(e->members);
}

int sue_incomplete_chk(struct_union_enum_st *st, const where *w)
{
	if(!sue_is_complete(st)){
		fold_had_error = 1;
		warn_at_print_error(w, "%s %s is incomplete", sue_str(st), st->spel);
		note_at(&st->where, "forward declared here");
		return 1;
	}

	UCC_ASSERT(st->foldprog == SUE_FOLDED_FULLY, "sizeof unfolded sue");
	if(st->primitive == type_enum)
		UCC_ASSERT(st->size > 0, "zero-sized enum");

	return 0;
}

unsigned sue_size(struct_union_enum_st *st, const where *w)
{
	if(sue_incomplete_chk(st, w))
		return 1; /* dummy size */

	return st->size; /* can be zero */
}

unsigned sue_align(struct_union_enum_st *st, const where *w)
{
	if(sue_incomplete_chk(st, w))
		return 1; /* dummy align */

	return st->align;
}

enum sue_szkind sue_sizekind(struct_union_enum_st *sue)
{
	sue_member **mi;

	if(!sue->members)
		return SUE_EMPTY;

	if(sue->primitive == type_enum)
		return SUE_NORMAL;

	for(mi = sue->members; mi && *mi; mi++){
		decl *d = (*mi)->struct_member;
		struct_union_enum_st *subsue;

		if(d->spel) /* not anon-bitfield, must have size */
			return SUE_NORMAL;

		if((subsue = type_is_s_or_u(d->ref))
		&& sue_sizekind(subsue) == SUE_NORMAL)
		{
			return SUE_NORMAL;
		}
	}

	return SUE_NONAMED;
}

struct_union_enum_st *sue_find_this_scope(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		struct_union_enum_st **i;
		for(i = stab->sues; i && *i; i++){
			struct_union_enum_st *st = *i;

			if(st->spel && !strcmp(st->spel, spel))
				return st;
		}

		if(symtab_is_transparent(stab))
			continue;
		break;
	}

	return NULL;
}

struct_union_enum_st *sue_find_descend(
		symtable *stab, const char *spel, int *const descended)
{
	if(descended)
		*descended = 0;

	for(; stab; stab = stab->parent){
		struct_union_enum_st *sue = sue_find_this_scope(stab, spel);
		if(sue)
			return sue;

		if(descended && symtab_is_transparent(stab))
			*descended = 1;
	}

	return NULL;
}

static void sue_get_decls(sue_member **mems, sue_member ***pds)
{
	for(; mems && *mems; mems++){
		decl *d = (*mems)->struct_member;

		if(d->spel){
			dynarray_add(pds, *mems);
		}else{
			/* either an anonymous struct/union OR a bitfield */
			struct_union_enum_st *sub = type_is_s_or_u(d->ref);

			if(sub)
				sue_get_decls(sub->members, pds);
		}
	}
}

static int decl_spel_cmp(const void *pa, const void *pb)
{
	const sue_member *a = *(sue_member *const *)pa,
	                 *b = *(sue_member *const *)pb;

	return strcmp(a->struct_member->spel, b->struct_member->spel);
}

sue_member *sue_member_from_decl(decl *d)
{
	sue_member *sm = umalloc(sizeof *sm);
	sm->struct_member = d;
	return sm;
}

struct_union_enum_st *sue_predeclare(
		struct symtable *scope,
		/*consumed*/char *spel,
		enum type_primitive prim,
		const where *loc)
{
	struct_union_enum_st *sue;

	if(spel)
		assert(!sue_find_this_scope(scope, spel));

	sue = sue_new(prim, loc, spel);

	symtab_add_sue(scope, sue);

	sue->membs_progress = SUE_MEMBS_NO;

	return sue;
}

void sue_define(struct_union_enum_st *sue, sue_member **members)
{
	sue->membs_progress = SUE_MEMBS_COMPLETE;

	assert(!sue->members);
	sue->members = members;
}

void sue_member_init_dup_check(sue_member **members)
{
	sue_member **decls = NULL;
	int i;

	sue_get_decls(members, &decls);

	qsort(decls,
			dynarray_count(decls), sizeof *decls,
			decl_spel_cmp);

	for(i = 0; decls && decls[i]; i++){
		decl *d2, *d = decls[i]->struct_member;

		if(d->bits.var.init.dinit){
			warn_at_print_error(&d->where, "member %s is initialised", d->spel);
			fold_had_error = 1;
		}

		if(decls[i + 1]
				&& (d2 = decls[i + 1]->struct_member,
					!strcmp(d->spel, d2->spel)))
		{
			char buf[WHERE_BUF_SIZ];

			warn_at_print_error(&d2->where, "duplicate member %s (from %s)",
					d->spel, where_str_r(buf, &d->where));
			fold_had_error = 1;
		}
	}

	dynarray_free(sue_member **, decls, NULL);
}

sue_member *sue_drop(struct_union_enum_st *sue, sue_member **pos)
{
	sue_member *ret = *pos;

	const size_t n = sue_nmembers(sue);
	size_t i = pos - sue->members;

	for(; i < n - 1; i++)
		sue->members[i] = sue->members[i + 1];
	sue->members[i] = NULL;

	return ret;
}

static void *sue_member_find(
		struct_union_enum_st *sue, const char *spel, unsigned *extra_off,
		struct_union_enum_st **pin)
{
	sue_member **mi;

	if(pin)
		*pin = NULL;

	for(mi = sue->members; mi && *mi; mi++){
		if(sue->primitive == type_enum){
			enum_member *em = (*mi)->enum_member;

			if(!strcmp(spel, em->spel))
				return em;

		}else{
			struct_union_enum_st *sub;
			decl *d = (*mi)->struct_member;
			char *sp = d->spel;

			if(sp){
				if(!strcmp(sp, spel))
					return d;

			}else if((sub = type_is_s_or_u(d->ref))){
				/* C11 anonymous struct/union */
				decl *dsub = NULL;
				decl *tdef;
				const int allow_tag = FOPT_TAG_ANON_STRUCT_EXT(&cc1_fopt);

				/* don't check spel - <anon struct ...> etc */
				if(!(allow_tag || sub->anon))
					continue;

				if((cc1_fopt.plan9_extensions)
				&& (tdef = type_is_tdef(d->ref))
				&& !strcmp(tdef->spel, spel))
				{
					dsub = tdef;
				}

				if(!dsub)
					dsub = sue_member_find(sub, spel, extra_off, pin);

				if(dsub){
					if(pin)
						*pin = sub;
					*extra_off += d->bits.var.struct_offset;
					return dsub;
				}
			}
		}
	}

	return NULL;
}

void enum_member_search_nodescend(
		enum_member **const pm,
		struct_union_enum_st **const psue,
		symtable *stab,
		const char *spel)
{
	struct_union_enum_st **i;

	for(i = stab->sues; i && *i; i++){
		struct_union_enum_st *e = *i;

		if(e->primitive == type_enum){
			enum_member *emem = sue_member_find(e, spel,
					NULL /* safe - is enum */, NULL);

			if(emem){
				*pm = emem;
				*psue = e;
				return;
			}
		}
	}

	*pm = NULL;
	*psue = NULL;
}

int enum_has_value(struct_union_enum_st *en, integral_t val)
{
	sue_member **i;

	UCC_ASSERT(en->primitive == type_enum, "enum");

	for(i = en->members; i && *i; i++){
		enum_member *ent = (*i)->enum_member;
		integral_t ent_i = const_fold_val_i(ent->val);

		if(val == ent_i)
			return 1;
	}
	return 0;
}

decl *struct_union_member_find_sue(struct_union_enum_st *in, struct_union_enum_st *needle)
{
	sue_member **i;

	UCC_ASSERT(in->primitive != type_enum, "enum");

	for(i = in->members; i && *i; i++){
		decl *d = (*i)->struct_member;
		struct_union_enum_st *s = type_is_s_or_u(d->ref);

		if(s == needle)
			return d;
	}

	return NULL;
}

decl *struct_union_member_find(
		struct_union_enum_st *sue,
		const char *spel, unsigned *extra_off,
		struct_union_enum_st **pin)
{
	return sue_member_find(sue, spel, extra_off, pin);
}

decl *struct_union_member_at_idx(struct_union_enum_st *sue, int idx)
{
	int n = sue_nmembers(sue);
	/*
	 * not needed since there are checks in decl init code,
	 * but in case this is used elsewhere...
	 */
	if(idx >= n)
		return NULL;

	return sue->members[idx]->struct_member;
}

int struct_union_member_idx(struct_union_enum_st *sue, decl *member)
{
	int i;
	for(i = 0; sue->members[i]; i++)
		if(sue->members[i]->struct_member == member)
			return i;
	return -1;
}
