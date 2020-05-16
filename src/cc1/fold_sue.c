#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/where.h"
#include "../util/limits.h"
#include "../util/math.h"
#include "../util/macros.h"

#include "cc1.h"
#include "fold.h"
#include "sue.h"
#include "const.h"
#include "fopt.h"

#include "pack.h"
#include "defs.h"

#include "fold_sue.h"
#include "type_is.h"
#include "type_nav.h"

#include "ops/expr_val.h"

struct bitfield_state
{
	type *master_ty;
	unsigned current_off, first_off;
	unsigned current_limit;
};

struct pack_state
{
	decl *d;
	struct_union_enum_st *sue;
	sue_member **iter;
	unsigned sz, align;
};

static void struct_pack(
		decl *d, unsigned long *poffset, unsigned sz, unsigned align)
{
	unsigned long after_space;

	pack_next(poffset, &after_space, sz, align);
	/* offset is the end of the decl, after_space is the start */

	d->bits.var.struct_offset = after_space;
}

static void round_size_to_align(unsigned *const size, unsigned align)
{
	*size = pack_to_align(*size, align);
}

static void struct_pack_finish_bitfield(
		unsigned long *offset, struct bitfield_state *bitfield)
{
	if(bitfield->master_ty){
		unsigned master_ty_size = type_size(bitfield->master_ty, NULL);

		if(*offset % master_ty_size)
			*offset += master_ty_size - *offset % master_ty_size;
	}
}

static void fold_enum(struct_union_enum_st *en, symtable *stab)
{
	const int has_bitmask = !!attr_present(en->attr, attr_enum_bitmask);
	sue_member **i;
	int defval = has_bitmask;
	integral_t max = 0, min = 0;
	type *contained_ty;
	int is_signed = 0;

	for(i = en->members; i && *i; i++){
		enum_member *m = (*i)->enum_member;
		expr *e = m->val;
		integral_t v;
		int negative = 0;

		/* -1 because we can't do dynarray_add(..., 0) */
		if(e == (expr *)-1){

			m->val = expr_set_where(
					expr_new_val(defval),
					&en->where);

			FOLD_EXPR(m->val, stab);

			v = defval;
			if(negative && v == 0){
				negative = 0;
			}

		}else{
			numeric n;
			int oob;

			m->val = FOLD_EXPR(e, stab);

			if(fold_check_expr(e,
					FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I,
					"enum member"))
			{
				continue;
			}

			const_fold_integral(e, &n);

			v = n.val.i;
			if(n.suffix & VAL_UNSIGNED)
				negative = 0;
			else
				negative = (sintegral_t)v < 0;

			/* enum constants must have type representable in 'int' */
			if(negative)
				oob = (sintegral_t)v < UCC_INT_MIN || (sintegral_t)v > UCC_INT_MAX;
			else
				oob = v > UCC_INT_MAX; /* int max - stick to int, not uint */

			if(oob){
				cc1_warn_at(&m->where,
						overlarge_enumerator_int,
						negative
						? "enumerator value %" NUMERIC_FMT_D " out of 'int' range"
						: "enumerator value %" NUMERIC_FMT_U " out of 'int' range",
						v);
			}
		}

		/* overflow here is a violation */
		if(v == UCC_INT_MAX && i[1] && i[1]->enum_member->val == (expr *)-1){
			m = i[1]->enum_member;
			warn_at_print_error(&m->where, "overflow for enum member %s::%s",
					en->spel, m->spel);
			fold_had_error = 1;
		}

		if(negative ?
		(sintegral_t)v < (sintegral_t)min
		: v > max)
		{
			if(negative)
				min = v;
			else
				max = v;
		}
		is_signed |= negative;

		defval = has_bitmask ? v << 1 : v + 1;
	}

	if(cc1_fopt.short_enums){
		unsigned bits = (MAX(log2ll(round2(-min + 1)), log2ll(round2(max + 1))));

		/* bits needs to be a power of 2 since those are the only word sizes supported */
		bits = round2(bits);

		if(bits < 8)
			bits = 8;

		contained_ty = type_nav_MAX_FOR(cc1_type_nav, bits / 8, is_signed);
	}else{
		contained_ty = type_nav_btype(cc1_type_nav, type_int);
	}

	en->size = type_size(contained_ty, NULL);
	en->align = type_align(contained_ty, NULL);
	round_size_to_align(&en->size, en->align);
}

static int fold_sue_check_unnamed(
		decl *d,
		struct_union_enum_st *sue,
		struct_union_enum_st *sub_sue,
		sue_member ***const pi)
{
	if(!d->spel){
		/* if the decl doesn't have a name, it's
		 * a useless decl, unless it's an anon struct/union
		 * or a bitfield
		 */
		if(d->bits.var.field_width){
			/* fine */
		}else if(sub_sue){
			/* anon */
			int drop_member = 0;

			switch(sue_anonext_type(d, sub_sue)){
				case SUE_ANONEXT_ALLOW:
					/*
					 * cc -fms-extensions/-fplan9-extensions
					 * struct A { ... };
					 * struct B { struct A; }; // struct B contains all of struct A's members
					 *            ^~~~~~~~
					 */
					cc1_warn_at(&d->where,
							unnamed_struct_memb_ext_tagged,
							"tagged struct '%s' is a Microsoft/Plan 9 extension",
							decl_to_str(d));
					break;

				case SUE_ANONEXT_DENY:
					/*
					 * struct A { ... };
					 * struct B { struct A; }; // declaration does not declare anything
					 *            ^~~~~~~~
					 */
					drop_member = 1;

					cc1_warn_at(&d->where,
							unnamed_struct_memb_ignored,
							"unnamed member '%s' ignored (untagged %swould be accepted in C11)",
							decl_to_str(d),
							type_is_tdef(d->ref) ? "and untypedef'd " : "");
					break;

				case SUE_ANONEXT_ALLOW_C11:
					if(cc1_std < STD_C11){
						/*
						 * struct B {
						 *   struct { ... }; // struct B contains all of anon struct's members
						 *          ^~~~~~~~
						 * };
						 */
						cc1_warn_at(&d->where,
								unnamed_struct_memb_ext_c11,
								"unnamed member '%s' is a C11 extension",
								decl_to_str(d));
					}
					break;
			}

			if(drop_member){
				/* drop the decl */
				sue_member *dropped = sue_drop(sue, *pi);
				--*pi;
				decl_free(dropped->struct_member);
				free(dropped);
				return 1;
			}
		}
	}
	return 0;
}

static int fold_sue_check_vm(decl *d)
{
	if(type_is_variably_modified(d->ref)){
		/* C99 6.7.6.2
		 * ... all identifiers declared with a VM type have to be ordinary
		 * identifiers and cannot, therefore, be members of structures or
		 * unions
		 * */
		fold_had_error = 1;
		warn_at_print_error(
				&d->where,
				"member has variably modifed type '%s'",
				type_to_str(d->ref));

		return 1;
	}
	return 0;
}

static void fold_sue_calc_fieldwidth(
		struct pack_state *pack_state,
		int *const realign_next,
		unsigned long *const offset,
		struct bitfield_state *const bitfield)
{
	const unsigned bits = const_fold_val_i(pack_state->d->bits.var.field_width);
	decl *const d = pack_state->d;

	/* don't affect sz_max or align_max */
	pack_state->sz = pack_state->align = 0;

	d->bits.var.first_bitfield = bitfield->current_off == 0;
	if(d->bits.var.first_bitfield)
		assert(bitfield->current_limit == 0);

	if(bits == 0){
		/* align next field / treat as new bitfield
		 * note we don't pad here - we don't want to
		 * take up any space with this field
		 */
		*realign_next = 1;

		/* also set struct_offset for 0-len bf, for pad reasons */
		d->bits.var.struct_offset = *offset;

	}else if(*realign_next
	|| !bitfield->current_off
	|| (bitfield->current_limit && bitfield->current_off + bits > bitfield->current_limit))
	{
		if(*realign_next || bitfield->current_off){
			if(!*realign_next){
				/* bitfield overflow - repad */
				cc1_warn_at(&d->where,
						bitfield_boundary,
						"bitfield overflow (%d + %d > %d) - "
						"moved to next boundary", bitfield->current_off, bits,
						bitfield->current_limit);
			}

			*realign_next = 0;
			bitfield->current_off = 0;

			if(bitfield->master_ty){
				/* round offset up to master_ty's size */
				struct_pack_finish_bitfield(offset, bitfield);
			}
		}

		bitfield->master_ty = d->ref;
		bitfield->current_limit = CHAR_BIT * type_size(d->ref, &d->where);

		d->bits.var.bitfield_master_ty = bitfield->master_ty;

		/* Get some initial padding.
		 * Note that we want to affect the align_max
		 * of the struct and the size of this field
		 */
		decl_size_align_inc_bitfield(d, &pack_state->sz, &pack_state->align);
		round_size_to_align(&pack_state->sz, pack_state->align);

		/* we are onto the beginning of a new group */
		struct_pack(d, offset, pack_state->sz, pack_state->align);
		bitfield->first_off = d->bits.var.struct_offset;

		/* now that we've done the struct packing w.r.t. bitfield size, we change
		 * pack_state->align to the align of the declared member type itself, to
		 * affect the struct's alignment (and also tail padding, etc) */
		pack_state->align = type_align(d->ref, NULL);

	}else{
		/* mirror previous bitfields' offset in the struct
		 * difference is in .struct_offset_bitfield
		 */
		d->bits.var.struct_offset = bitfield->first_off;
		d->bits.var.bitfield_master_ty = bitfield->master_ty;
	}

	d->bits.var.struct_offset_bitfield = bitfield->current_off;
	bitfield->current_off += bits; /* allowed to go above sizeof(int) */

	if(bitfield->current_off == bitfield->current_limit){
		/* exactly reached the limit, reset bitfield indexing */
		bitfield->current_off = 0;

		struct_pack_finish_bitfield(offset, bitfield);
	}
}

static void populate_size_align(struct pack_state *pack_state)
{
	decl *d = pack_state->d;
	if(!type_is_incomplete_array(d->ref))
		pack_state->sz = decl_size(d);
	else
		pack_state->sz = 0;

	pack_state->align = decl_align(d);
	/* don't round size up to align here, we're still packing inside a struct */
}

static void fold_sue_calc_normal(struct pack_state *const pack_state)
{
	decl *const d = pack_state->d;

	populate_size_align(pack_state);

	if(type_is_incomplete_array(d->ref)){
		if(pack_state->iter[1])
			die_at(&d->where, "flexible array not at end of struct");
		else if(pack_state->sue->primitive != type_struct)
			die_at(&d->where, "flexible array in a %s", sue_str(pack_state->sue));
		else if(pack_state->iter == pack_state->sue->members) /* nothing currently */
			cc1_warn_at(&d->where, flexarr_only,
					"struct with just a flex-array is an extension");

		pack_state->sue->flexarr = 1;
		pack_state->sz = 0; /* not counted in struct size */
	}
}

static void fold_sue_apply_normal_offset(
		struct pack_state *pack_state,
		unsigned long *const offset,
		struct bitfield_state *const bitfield)
{
	decl *const d = pack_state->d;
	const int prev_offset = *offset;
	int pad;

	if(bitfield->current_off){
		/* we automatically pad on the next struct_pack,
		 * don't struct_pack() here */
		bitfield->current_off = 0;
	}

	struct_pack(d, offset, pack_state->sz, pack_state->align);

	pad = d->bits.var.struct_offset - prev_offset;
	if(pad){
		cc1_warn_at(&d->where, pad,
				"padding '%s' with %d bytes to align '%s'",
				pack_state->sue->spel, pad, decl_to_str(d));
	}
}

static void fold_sue_calc_substrut(
		struct pack_state *pack_state,
		struct_union_enum_st *sub_sue,
		symtable *stab,
		int *const submemb_const)
{
	char desc[32];

	snprintf(desc, sizeof desc, "nested in %s",
			type_primitive_to_str(pack_state->sue->primitive));

	fold_check_embedded_flexar(
			sub_sue, &pack_state->d->where,
			desc);

	if(sub_sue != pack_state->sue){
		fold_sue(sub_sue, stab);

		if(sub_sue->contains_const)
			*submemb_const = 1;
	}

	/* should've been caught by incompleteness checks */
	UCC_ASSERT(sub_sue != pack_state->sue, "nested %s", sue_str(pack_state->sue));

	if(sub_sue->flexarr && pack_state->iter[1]){
		cc1_warn_at(&pack_state->d->where,
				flexarr_embed,
				"embedded struct with flex-array not final member");
	}

	populate_size_align(pack_state);
}

static void check_sue_align_attr(struct_union_enum_st *sue, symtable *stab)
{
	sue->align = fold_get_max_align_attribute(sue->attr, stab, sue->align);
	round_size_to_align(&sue->size, sue->align);
}

void fold_sue(struct_union_enum_st *const sue, symtable *stab)
{
	if(sue->foldprog != SUE_FOLDED_NO || sue->membs_progress != SUE_MEMBS_COMPLETE)
		return;
	sue->foldprog = SUE_FOLDED_PARTIAL;

	if(sue->primitive == type_enum){
		fold_enum(sue, stab);

	}else{
		unsigned align_max = 1;
		unsigned sz_max = 0;
		unsigned long offset = 0;
		int realign_next = 0;
		int submemb_const = 0;
		struct bitfield_state bitfield;
		sue_member **i;
		const int packed = !!attr_present(sue->attr, attr_packed);

		memset(&bitfield, 0, sizeof bitfield);

		if(cc1_fopt.dump_layouts)
			fprintf(stderr, "Record layout for %s:\n", sue->spel);

		for(i = sue->members; i && *i; i++){
			decl *d = (*i)->struct_member;
			struct_union_enum_st *sub_sue = type_is_s_or_u(d->ref);
			struct pack_state pack_state;

			fold_decl_maybe_member(d, stab, 1);

			if(fold_sue_check_vm(d))
				continue;

			if(fold_sue_check_unnamed(d, sue, sub_sue, &i))
				continue;

			if(!type_is_complete(d->ref)
			&& !type_is_incomplete_array(d->ref)) /* allow flexarrays */
			{
				warn_at_print_error(&d->where, "incomplete field '%s'", decl_to_str(d));
				fold_had_error = 1;
				continue;
			}

			if(type_is_const(d->ref))
				submemb_const = 1;

			pack_state.d = d;
			pack_state.sue = sue;
			pack_state.iter = i;

			if(sub_sue){
				fold_sue_calc_substrut(&pack_state, sub_sue, stab, &submemb_const);

			}else if(d->bits.var.field_width){
				fold_sue_calc_fieldwidth(
						&pack_state,
						&realign_next, &offset,
						&bitfield);

			}else{
				fold_sue_calc_normal(&pack_state);
			}

			if(packed || attribute_present(d, attr_packed))
				pack_state.align = 1;

			if(sue->primitive == type_struct && !d->bits.var.field_width){
				fold_sue_apply_normal_offset(&pack_state, &offset, &bitfield);
			}

			if(cc1_fopt.dump_layouts){
				fprintf(stderr, " %2u", d->bits.var.struct_offset);

				if(d->bits.var.field_width){
					const unsigned bits = const_fold_val_i(d->bits.var.field_width);

					fprintf(stderr, ":%2u-%-2u ",
							d->bits.var.struct_offset_bitfield,
							d->bits.var.struct_offset_bitfield + (bits == 0 ? 0 : bits - 1));
				}else{
					fprintf(stderr, "       ");
				}

				fprintf(stderr, "| .%-10s", d->spel ? d->spel : "<anon>");
				if(type_is_complete(d->ref)){
					fprintf(stderr, " size=%u align=%u",
						type_size(d->ref, NULL),
						decl_align(d));
				}
				fprintf(stderr, "\n");
			}

			if(pack_state.align > align_max)
				align_max = pack_state.align;
			if(pack_state.sz > sz_max)
				sz_max = pack_state.sz;
		}

		sue->contains_const = submemb_const;

		switch(sue_sizekind(sue)){
				const char *warn;
			case SUE_NORMAL:
				break;
			case SUE_EMPTY:
				warn = "is empty";
				goto warn;
			case SUE_NONAMED:
				warn = "has no named members";
warn:
				cc1_warn_at(NULL, empty_struct, "%s %s", sue_str_type(sue->primitive), warn);
		}

		sue->align = align_max;
		sue->size = pack_to_align(
				sue->primitive == type_struct ? offset : sz_max,
				align_max);
		round_size_to_align(&sue->size, sue->align);

		check_sue_align_attr(sue, stab);

		if(cc1_fopt.dump_layouts)
			fprintf(stderr, "          | record size=%u align=%u\n", sue->size, sue->align);
	}

	sue->foldprog = SUE_FOLDED_FULLY;
}
