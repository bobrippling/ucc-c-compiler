#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../util/where.h"
#include "../../util/platform.h"
#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/dynmap.h"
#include "../../util/alloc.h"

#include "../str.h"

#include "../expr.h"
#include "../stmt.h"
#include "../const.h"
#include "../funcargs.h"
#include "../sue.h"
#include "../type.h"
#include "../funcargs.h"
#include "../type_is.h"
#include "../vla.h"
#include "../cc1_out_ctx.h"

#include "asm.h" /* cc_out[] */

#include "../defs.h" /* CHAR_BIT */
#include "../../as_cfg.h" /* section names, private label */

#include "leb.h" /* leb128 */

#include "lbl.h"
#include "dbg.h"
#include "write.h" /* dbg_add_file */
#include "val.h"
#include "backend.h" /* REG_BP */
#include "blk.h" /* .lbl */
#include "dbg_lbl.h"

#define DEBUG_TYPE_SKIP type_skip_non_tdefs_consts
#define DEBUG_TYPE_HASH type_hash_skip_nontdefs_consts


#define DW_TAGS                        \
	X(DW_TAG_compile_unit, 0x11)         \
	X(DW_TAG_subprogram, 0x2e)           \
	X(DW_TAG_base_type, 0x24)            \
	X(DW_TAG_typedef, 0x16)              \
	X(DW_TAG_pointer_type, 0xf)          \
	X(DW_TAG_array_type, 0x1)            \
	X(DW_TAG_subrange_type, 0x21)        \
	X(DW_TAG_const_type, 0x26)           \
	X(DW_TAG_volatile_type, 0x35)        \
	X(DW_TAG_restrict_type, 0x37)        \
	X(DW_TAG_subroutine_type, 0x15)      \
	X(DW_TAG_enumeration_type, 0x4)      \
	X(DW_TAG_enumerator, 0x28)           \
	X(DW_TAG_structure_type, 0x13)       \
	X(DW_TAG_union_type, 0x17)           \
	X(DW_TAG_variable, 0x34)             \
	X(DW_TAG_formal_parameter, 0x5)      \
	X(DW_TAG_unspecified_parameters, 0x18) \
	X(DW_TAG_member, 0xd)                \
	X(DW_TAG_lexical_block, 0x0b)        \
	X(DW_TAG_inlined_subroutine, 0x1d)

#define DW_ATTRS                       \
	X(DW_AT_data_member_location, 0x38)  \
	X(DW_AT_external, 0x3f)              \
	X(DW_AT_byte_size, 0xb)              \
	X(DW_AT_encoding, 0x3e)              \
	X(DW_AT_bit_offset, 0xc)             \
	X(DW_AT_bit_size, 0xd)               \
	X(DW_AT_decl_file, 0x3a)             \
	X(DW_AT_decl_line, 0x3b)             \
	X(DW_AT_stmt_list, 0x10)             \
	X(DW_AT_name, 0x3)                   \
	X(DW_AT_language, 0x13)              \
	X(DW_AT_low_pc, 0x11)                \
	X(DW_AT_high_pc, 0x12)               \
	X(DW_AT_producer, 0x25)              \
	X(DW_AT_comp_dir, 0x1b)              \
	X(DW_AT_type, 0x49)                  \
	X(DW_AT_sibling, 0x1)                \
	X(DW_AT_lower_bound, 0x22)           \
	X(DW_AT_upper_bound, 0x2f)           \
	X(DW_AT_prototyped, 0x27)            \
	X(DW_AT_location, 0x2)               \
	X(DW_AT_const_value, 0x1c)           \
	X(DW_AT_accessibility, 0x32)         \
	X(DW_AT_specification, 0x2b0)        \
	X(DW_AT_inline, 0x20)                \
	X(DW_AT_abstract_origin, 0x31)       \
	X(DW_AT_call_file, 0x58)             \
	X(DW_AT_call_line, 0x59)

#define DW_ENCS            \
	X(DW_FORM_addr, 0x1)     \
	/* synthesized: */       \
	X(DW_FORM_addr_lbl, 0xff)\
	X(DW_FORM_data1, 0xb)    \
	X(DW_FORM_data2, 0x5)    \
	X(DW_FORM_data4, 0x6)    \
	X(DW_FORM_data8, 0x7)    \
	X(DW_FORM_ULEB, 0x2)     \
	X(DW_FORM_ADDR4, 0x3)    \
	X(DW_FORM_string, 0x8)   \
	X(DW_FORM_ref4, 0x13)    \
	X(DW_FORM_flag, 0xc)     \
	X(DW_FORM_block1, 0xa)

#define DW_OPS               \
	X(DW_OP_plus_uconst, 0x23) \
	X(DW_OP_addr, 0x3)         \
	X(DW_OP_breg6, 0x76)       \
	X(DW_OP_deref, 0x6)

enum dwarf_tag
{
#define X(nam, val) nam = val,
	DW_TAGS
#undef X
};

enum dwarf_attribute
{
#define X(nam, val) nam = val,
	DW_ATTRS
#undef X
};

enum dwarf_attr_encoding
{
#define X(nam, val) nam = val,
	DW_ENCS
#undef X
};
typedef integral_t form_data_t;

enum dwarf_block_ops
{
#define X(nam, val) nam = val,
	DW_OPS
#undef X
};

static const char *die_tag_to_str(enum dwarf_tag t)
{
	switch(t){
#define X(nam, val) case nam: return # nam;
		DW_TAGS
#undef X
	}
	return NULL;
}

static const char *die_attr_to_str(enum dwarf_attribute a)
{
	switch(a){
#define X(nam, val) case nam: return # nam;
		DW_ATTRS
#undef X
	}
	return NULL;
}

static const char *die_enc_to_str(enum dwarf_attr_encoding e)
{
	switch(e){
#define X(nam, val) case nam: return # nam;
		DW_ENCS
#undef X
	}
	return NULL;
}

static const char *die_op_to_str(enum dwarf_block_ops o)
{
	switch(o){
#define X(nam, val) case nam: return # nam;
		DW_OPS
#undef X
	}
	return NULL;
}

struct dwarf_block_ent
{
	enum
	{
		BLOCK_HEADER,
		BLOCK_LEB128_S,
		BLOCK_LEB128_U,
		BLOCK_ADDR_STR
	} type;

	union
	{
		long v;
		char *str;
	} bits;
};

struct dwarf_block
{
	struct dwarf_block_ent *ents;
	int cnt;
};

struct DIE
{
	enum dwarf_tag tag;
	unsigned long locn;
	unsigned long abbrev_code;

	struct DIE_attr
	{
		enum dwarf_attribute attr;
		enum dwarf_attr_encoding enc;

		union
		{
			struct dwarf_block *blk;
			struct DIE *die_xref;
			char *str;
			long value;
			struct out_dbg_lbl *lbl;
		} bits;
	} **attrs;

	struct DIE **children, *parent;
};

struct DIE_compile_unit
{
	struct DIE die;
	struct out_dbg_filelist **pfilelist;
	dynmap *types_to_dies;
};

enum dwarf_misc
{
	DW_ACCESS_public = 0x1,

	DW_CHILDREN_no = 0,
	DW_CHILDREN_yes = 1,

	DW_INL_inlined = 1,

	DW_LANG_C89 = 0x1,
	DW_LANG_C99 = 0xc,
	DW_LANG_C11 = 0x1d
};

enum dwarf_encoding
{
	DW_ATE_boolean = 0x2,
	DW_ATE_float = 0x4,
	DW_ATE_signed = 0x5,
	DW_ATE_signed_char = 0x6,
	DW_ATE_unsigned = 0x7,
	DW_ATE_unsigned_char = 0x8,
};

static struct DIE *dwarf_suetype(
		struct DIE_compile_unit *cu,
		struct_union_enum_st *sue,
		type *suety);

static struct DIE *dwarf_type_die(
		struct DIE_compile_unit *cu,
		struct DIE *parent, type *ty);

static struct DIE *dwarf_tydie_new(
		struct DIE_compile_unit *cu,
		type *ty,
		enum dwarf_tag tag);

static struct DIE *dbg_create_decl_die(
		struct DIE_compile_unit *cu, decl *, const out_val *val);

static void dwarf_location_addr(struct dwarf_block_ent *locn_ents, decl *d);

static void dwarf_attr_decl(
		struct DIE_compile_unit *cu,
		struct DIE *in,
		decl *d,
		type *ty, int show_extern);

static void dwarf_die_free_r(struct DIE *die);

static void dwarf_free_children(struct DIE *parent)
{
	struct DIE **const ar = parent->children, **i;

	/* prevent recursive releases from freeing us again */
	parent->children = NULL;

	for(i = ar; i && *i; i++)
		dwarf_die_free_r(*i);

	free(ar);
}

static struct cc1_dbg_ctx *octx2dbg(out_ctx *octx)
{
	return &cc1_out_ctx_or_new(octx)->dbg;
}

static void dwarf_die_new_at(struct DIE *d, enum dwarf_tag tag)
{
	d->tag = tag;
}

static struct DIE *dwarf_die_new(enum dwarf_tag tag)
{
	struct DIE *d = umalloc(sizeof *d);
	dwarf_die_new_at(d, tag);
	return d;
}

static void dwarf_child(struct DIE *parent, struct DIE *child)
{
	if(child->parent)
		return;
	dynarray_add(&parent->children, child);
	child->parent = parent;
}

static void dwarf_children(struct DIE *parent, struct DIE **children)
{
	struct DIE **i;
	for(i = children; i && *i; i++)
		dwarf_child(parent, *i);

	free(children);
}

static void dwarf_die_free_1(struct DIE *die)
{
	struct DIE_attr **ai;

	for(ai = die->attrs; ai && *ai; ai++){
		struct DIE_attr *a = *ai;

		switch(a->enc){
			case DW_FORM_block1:
			{
				int i;

				for(i = 0; i < a->bits.blk->cnt; i++){
					struct dwarf_block_ent *e = &a->bits.blk->ents[i];
					if(e->type == BLOCK_ADDR_STR)
						free(e->bits.str);
				}
				free(a->bits.blk->ents);
				free(a->bits.blk);
				break;
			}

			case DW_FORM_ref4:
				/* die_xref - weak reference */
				break;

			case DW_FORM_addr:
			case DW_FORM_ADDR4:
				free(a->bits.str);
				break;

			case DW_FORM_addr_lbl:
				RELEASE(a->bits.lbl);
				break;

			case DW_FORM_ULEB:
			case DW_FORM_flag:
			case DW_FORM_data1:
			case DW_FORM_data2:
			case DW_FORM_data4:
			case DW_FORM_data8:
				/* just a value */
				break;

			case DW_FORM_string:
				free(a->bits.str);
				break;
		}

		free(a);
	}

	free(die->attrs);
	free(die);
}

static void dwarf_die_free_r(struct DIE *die)
{
	dwarf_free_children(die);

	dwarf_die_free_1(die);
}

static struct DIE_attr *dwarf_attr_lookup(
		struct DIE *die, enum dwarf_attribute attr)
{
	struct DIE_attr **ai;

	for(ai = die->attrs; ai && *ai; ai++){
		struct DIE_attr *a = *ai;

		if(a->attr == attr)
			return a;
	}

	return NULL;
}

static struct DIE *dwarf_global_lookup(
		struct DIE_compile_unit *cu, const char *spel)
{
	struct DIE **i;

	for(i = cu->die.children; i && *i; i++){
		struct DIE_attr *a = dwarf_attr_lookup(*i, DW_AT_name);

		if(a && !strcmp(a->bits.str, spel))
			return *i;
	}

	return NULL;
}

static void dwarf_attr(
		struct DIE *die,
		enum dwarf_attribute attr,
		enum dwarf_attr_encoding enc,
		void *data)
{
	struct DIE_attr *at = umalloc(sizeof *at);

	dynarray_add(&die->attrs, at);

	at->attr = attr;
	at->enc = enc;

	switch(enc){
		case DW_FORM_block1:
			at->bits.blk = data;
			break;
		case DW_FORM_ref4:
			at->bits.die_xref = data;
			break;
		case DW_FORM_addr:
			at->bits.str = data;
			break;
		case DW_FORM_addr_lbl:
			at->bits.lbl = RETAIN((struct out_dbg_lbl *)data);
			break;

		case DW_FORM_ADDR4:
			at->bits.str = data;
			break;

		case DW_FORM_ULEB:
		case DW_FORM_flag:
		case DW_FORM_data1:
		case DW_FORM_data2:
		case DW_FORM_data4:
		case DW_FORM_data8:
			at->bits.value = *(form_data_t *)data;

			if(enc == DW_FORM_ULEB){
				switch(leb128_length(at->bits.value, 0)){
					case 1: at->enc = DW_FORM_data1; break;
					case 2: at->enc = DW_FORM_data2; break;
					case 4: at->enc = DW_FORM_data4; break;
					case 8: at->enc = DW_FORM_data8; break;
					default: ucc_unreach();
				}
			}
			break;

		case DW_FORM_string:
			at->bits.str = str_add_escape(data, strlen(data));
			break;
	}
}

static struct DIE *dwarf_basetype(struct DIE_compile_unit *cu, type *ty)
{
	const enum type_primitive prim = ty->bits.type->primitive;
	form_data_t enc;
	struct DIE *tydie;

	switch(prim){
		case type__Bool:
			enc = DW_ATE_boolean;
			break;

		case type_nchar:
			if(type_primitive_is_signed(prim, 1)){
		case type_schar:
				enc = DW_ATE_signed_char;
			}else{
		case type_uchar:
				enc = DW_ATE_unsigned_char;
			}
			break;

		case type_short:
		case type_int:
		case type_long:
		case type_llong:
			enc = DW_ATE_signed;
			break;

		case type_ushort:
		case type_uint:
		case type_ulong:
		case type_ullong:
			enc = DW_ATE_unsigned;
			break;

		case type_float:
		case type_double:
		case type_ldouble:
			enc = DW_ATE_float;
			break;

		case type_void:
		case type_struct:
		case type_union:
		case type_enum:
		case type_unknown:
			ICE("bad type");
	}

	tydie = dwarf_tydie_new(cu, ty, DW_TAG_base_type);

	dwarf_attr(tydie, DW_AT_name,
			DW_FORM_string,
			(char *)type_primitive_to_str(prim));

	dwarf_attr(tydie, DW_AT_encoding,
			DW_FORM_data1, &enc);

	enc = type_primitive_size(prim);
	dwarf_attr(tydie, DW_AT_byte_size,
			DW_FORM_data4, &enc);

	return tydie;
}

static void dwarf_set_DW_AT_type(
		struct DIE *in,
		struct DIE_compile_unit *cu,
		struct DIE *parent,
		type *ty)
{
	struct DIE *tydie = dwarf_type_die(cu, parent, ty);

	if(ty->type == type_btype && ty->bits.type->primitive == type_void)
		assert(!tydie);

	if(tydie)
		dwarf_attr(in, DW_AT_type, DW_FORM_ref4, tydie);
}

/* this should only be called from dwarf_tydie_new() to ensure
 * there is exactly one type die per type */
static void dwarf_add_tydie(
		struct DIE_compile_unit *cu, type *ty, struct DIE *tydie)
{
	struct DIE *prev;
	int replaced_another;

	ty = DEBUG_TYPE_SKIP(ty);

	if(!cu->types_to_dies){
		cu->types_to_dies = dynmap_new(
				type *,
				type_eq_nontdef, /* necessary since we use type_hash_skip() */
				DEBUG_TYPE_HASH); /* attr/where aren't emitted */
	}

	prev = dynmap_set(type *, struct DIE *, cu->types_to_dies, ty, tydie);

	replaced_another = (prev && prev != tydie);

	/* prev should either be tydie (previously added) or null
	 * since if we have added the same type twice then we've got two different
	 * DIEs floating around that represent the same type
	 */
	UCC_ASSERT(!replaced_another, "replaced an unrelated type die in the map");
}

static struct DIE *dwarf_tydie_new(
		struct DIE_compile_unit *cu,
		type *ty,
		enum dwarf_tag tag)
{
	struct DIE *tydie = dwarf_die_new(tag);

	if(!tydie)
		return NULL; /* void */

	/* register immediately, so subsequent type lookups find
	 * this tydie, and don't create a new one.
	 *
	 * otherwise, we end up with two type dies floating around,
	 * which means one will replace the other in the type map
	 * (types_to_dies), meaning if there are any references to
	 * the ejected die, they'll try to use it even though it won't
	 * have its location set, causing all sorts of problems.
	 */
	dwarf_add_tydie(cu, ty, tydie);

	return tydie;
}

static struct DIE *dwarf_maybe_create_variadic_tag(funcargs *args)
{
	return args->variadic
		? dwarf_die_new(DW_TAG_unspecified_parameters)
		: NULL;
}

static struct DIE **dwarf_param_types(
		struct DIE_compile_unit *cu, funcargs *args)
{
	struct DIE **dieargs = NULL;
	decl **di;
	struct DIE *va_tag;

	for(di = args->arglist; di && *di; di++){
		decl *d = *di;

		dynarray_add(&dieargs, dbg_create_decl_die(cu, d, NULL));
	}

	va_tag = dwarf_maybe_create_variadic_tag(args);
	if(va_tag)
		dynarray_add(&dieargs, va_tag);

	return dieargs;
}

static struct DIE *dwarf_type_die(
		struct DIE_compile_unit *cu,
		struct DIE *parent, type *ty)
{
	/* search back and up for a type DIE */
	struct DIE *tydie;

	if(cu->types_to_dies){
		type *skipped_ty = DEBUG_TYPE_SKIP(ty);

		tydie = dynmap_get(type *, struct DIE *, cu->types_to_dies, skipped_ty);
		if(tydie)
			return tydie;
	}

	switch(ty->type){
		case type_auto:
			ICE("__auto_type");

		case type_btype:
		{
			struct_union_enum_st *sue = ty->bits.type->sue;

			if(sue){
				tydie = dwarf_suetype(cu, sue, ty);

			}else{
				if(ty->bits.type->primitive == type_void)
					return NULL;

				tydie = dwarf_basetype(cu, ty);
			}
			break;
		}

		case type_tdef:
			if(ty->bits.tdef.decl){
				decl *d = ty->bits.tdef.decl;

				/* we map the actual typedef type onto the tydie,
				 * not the type the typedef uses */
				tydie = dwarf_tydie_new(
						cu,
						/*not: d->ref, but:*/ty,
						DW_TAG_typedef);

				dwarf_attr(tydie, DW_AT_name, DW_FORM_string, d->spel);

				dwarf_set_DW_AT_type(tydie, cu, parent, d->ref);

			}else{
				/* skip typeof() */
				tydie = dwarf_type_die(cu, parent,
						ty->bits.tdef.type_of->tree_type);
			}
			break;

		case type_block: /* act as if type_ptr */
		case type_ptr:
		{
			form_data_t sz = platform_word_size();

			tydie = dwarf_tydie_new(cu, ty, DW_TAG_pointer_type);

			dwarf_attr(tydie, DW_AT_byte_size,
					DW_FORM_data4, &sz);

			dwarf_set_DW_AT_type(tydie, cu, parent, ty->ref);
			break;
		}

		case type_func:
		{
			form_data_t flag = 1;

			tydie = dwarf_tydie_new(cu, ty, DW_TAG_subroutine_type);

			dwarf_set_DW_AT_type(tydie, cu, parent, ty->ref);

			dwarf_attr(tydie, DW_AT_prototyped, DW_FORM_flag, &flag);

			dwarf_children(tydie, dwarf_param_types(cu, ty->bits.func.args));
			break;
		}

		case type_array:
		{
			int have_sz = !!ty->bits.array.size;
			struct DIE *szdie;

			tydie = dwarf_tydie_new(cu, ty, DW_TAG_array_type);

			dwarf_set_DW_AT_type(tydie, cu, parent, ty->ref);

			szdie = dwarf_die_new(DW_TAG_subrange_type);
			if(have_sz){
				form_data_t sz = ty->bits.array.is_vla
					? 0 : const_fold_val_i(ty->bits.array.size);

				/*dwarf_attr(szdie, DW_AT_lower_bound, DW_FORM_data4, 0);*/

				if(sz > 0){
					dwarf_attr(szdie, DW_AT_upper_bound,
							DW_FORM_data4,
							(--sz, &sz));
				}
			}

			dwarf_child(tydie, szdie);
			break;
		}

		case type_cast:
		{
			if(ty->bits.cast.is_signed_cast){
				/* skip */
				tydie = dwarf_type_die(cu, parent, ty->ref);
			}else{
				/* due to how types map to tydies,
				 * we can only have a single qualifier */
				enum type_qualifier q = ty->bits.cast.qual;
				enum dwarf_tag tag;

				if(q & qual_const)
					tag = DW_TAG_const_type;
				else if(q & qual_volatile)
					tag = DW_TAG_volatile_type;
				else if(q & qual_restrict)
					tag = DW_TAG_restrict_type;
				else
					ucc_unreach(NULL);

				tydie = dwarf_tydie_new(cu, ty, tag);
				dwarf_set_DW_AT_type(tydie, cu, parent, ty->ref);
			}
			break;
		}

		case type_attr:
		case type_where:
			/* skip */
			tydie = dwarf_type_die(cu, parent, ty->ref);
			break;
	}


	return tydie;
}

static struct DIE *dwarf_sue_header(
		struct DIE_compile_unit *cu,
		struct_union_enum_st *sue,
		enum dwarf_tag dwarf_tag,
		type *suety)
{
	struct DIE *suedie = dwarf_tydie_new(cu, suety, dwarf_tag);

	if(!sue->anon)
		dwarf_attr(suedie, DW_AT_name, DW_FORM_string, sue->spel);

	if(sue_complete(sue)){
		form_data_t sz = sue_size(sue, NULL);

		dwarf_attr(suedie, DW_AT_byte_size,
				DW_FORM_data4, &sz);
	}

	return suedie;
}

static struct DIE *dwarf_suetype(
		struct DIE_compile_unit *cu,
		struct_union_enum_st *sue,
		type *suety)
{
	struct DIE *suedie;

	switch(sue->primitive){
		default:
			ucc_unreach(NULL);

		case type_enum:
		{
			sue_member **i;

			suedie = dwarf_sue_header(cu, sue, DW_TAG_enumeration_type, suety);

			/* enumerators */
			for(i = sue->members; i && *i; i++){
				enum_member *emem = (*i)->enum_member;
				struct DIE *memdie = dwarf_die_new(DW_TAG_enumerator);
				form_data_t sz = const_fold_val_i(emem->val);

				dwarf_attr(memdie,
						DW_AT_name, DW_FORM_string,
						emem->spel);

				dwarf_attr(memdie,
						DW_AT_const_value, DW_FORM_data4,
						&sz);

				dwarf_child(suedie, memdie);
			}
			break;
		}

		case type_union:
		case type_struct:
		{
			sue_member **si;

			suedie = dwarf_sue_header(
					cu,
					sue,
					sue->primitive == type_struct
					? DW_TAG_structure_type
					: DW_TAG_union_type,
					suety);

			/* members */
			for(si = sue->members; si && *si; si++){
				decl *dmem = (*si)->struct_member;
				struct DIE *memdie;

				struct dwarf_block *offset;
				struct dwarf_block_ent *blkents;

				if(DECL_IS_ANON_BITFIELD(dmem)){
					/* skip, otherwise dwarf thinks this decl's a field and messes up */
					continue;
				}

				memdie = dwarf_die_new(DW_TAG_member);

				dwarf_child(suedie, memdie);

				if(dmem->spel){
					/* could be anonymous sub-struct/union */
					dwarf_attr(memdie,
							DW_AT_name, DW_FORM_string,
							dmem->spel);
				}

				dwarf_set_DW_AT_type(memdie, cu, NULL, dmem->ref);

				blkents = umalloc(2 * sizeof *blkents);

				blkents[0].type = BLOCK_HEADER;
				blkents[0].bits.v = DW_OP_plus_uconst;
				blkents[1].type = BLOCK_LEB128_U;
				blkents[1].bits.v = dmem->bits.var.struct_offset;

				offset = umalloc(sizeof *offset);
				offset->cnt = 2;
				offset->ents = blkents;

				dwarf_attr(memdie,
						DW_AT_data_member_location, DW_FORM_block1,
						offset);

				/* bitfield */
				if(dmem->bits.var.field_width){
					form_data_t width = const_fold_val_i(dmem->bits.var.field_width);
					form_data_t whole_sz = type_size(dmem->ref, NULL);

					/* address of top-end */
					form_data_t off =
						(whole_sz * CHAR_BIT)
						- (width + dmem->bits.var.struct_offset_bitfield);

					dwarf_attr(memdie,
							DW_AT_bit_offset, DW_FORM_data1,
							&off);

					dwarf_attr(memdie,
							DW_AT_bit_size, DW_FORM_data1,
							&width);
				}
			}
			break;
		}
	}

	return suedie;
}

static int dbg_get_val_location(const out_val *v, long *const offset)
{
	switch(v->type){
		case V_REG_SPILT:
		case V_REG:
			if(v->bits.regoff.reg.idx == REG_BP){
				*offset = v->bits.regoff.offset;
				return 1;
			}
			/* fall */
		default:
			return 0;
	}
}

static void dbg_add_sym_location(struct DIE *param, const out_val *v)
{
	struct dwarf_block *locn;
	struct dwarf_block_ent *locn_data;
	long offset;

	if(!dbg_get_val_location(v, &offset))
		return;

	locn = umalloc(sizeof *locn);
	locn_data = umalloc(2 * sizeof *locn_data);

	locn_data[0].type = BLOCK_HEADER;
	locn_data[0].bits.v = DW_OP_breg6; /* rbp */
	locn_data[1].type = BLOCK_LEB128_S;
	locn_data[1].bits.v = offset;

	locn->cnt = 2;
	locn->ents = locn_data;

	dwarf_attr(param, DW_AT_location, DW_FORM_block1, locn);
}

static struct DIE *dbg_create_decl_die_arg(
		struct DIE_compile_unit *cu, decl *d, const out_val *val)
{
	struct DIE *param = dwarf_die_new(DW_TAG_formal_parameter);

	dwarf_set_DW_AT_type(param, cu, NULL, d->ref);

	if(d->spel){
		dwarf_attr(param, DW_AT_name, DW_FORM_string, d->spel);

		if(val)
			dbg_add_sym_location(param, val);
	}

	return param;
}

static struct DIE *dbg_create_decl_die_local(
		struct DIE_compile_unit *cu, decl *d, const out_val *val)
{
	struct DIE *var = dwarf_die_new(DW_TAG_variable);
	long offset;

	assert(d->sym);
	assert(val && "local variable without out_val");
	if(dbg_get_val_location(val, &offset)){
		struct dwarf_block_ent *locn_ents;
		struct dwarf_block *locn;
		const int vla = type_is_variably_modified(d->ref);

		locn = umalloc(sizeof *locn);
		locn->cnt = 2 + vla;

		locn_ents = umalloc(locn->cnt * sizeof *locn_ents);
		locn->ents = locn_ents;

		switch(d->sym->type){
			case sym_local:
				locn_ents[0].type = BLOCK_HEADER;
				locn_ents[0].bits.v = DW_OP_breg6; /* rbp */

				locn_ents[1].type = BLOCK_LEB128_S;
				locn_ents[1].bits.v = offset;

				if(vla){
					locn_ents[2].type = BLOCK_LEB128_S;
					locn_ents[2].bits.v = DW_OP_deref;
				}
				break;

			case sym_global:
				dwarf_location_addr(locn_ents, d);
				break;

			case sym_arg:
				/* ignore arguments in local scope:
				 * may be entering a block's code scope */
				break;
		}

		dwarf_attr(var, DW_AT_location, DW_FORM_block1, locn);
	}

	dwarf_attr_decl(cu, var, d, d->ref, /*show_extern:*/0);

	return var;
}

static struct DIE *dbg_create_decl_die(
		struct DIE_compile_unit *cu,
		decl *d,
		const out_val *val)
{
	if(!d->sym || d->sym->type == sym_arg)
		return dbg_create_decl_die_arg(cu, d, val);

	return dbg_create_decl_die_local(cu, d, val);
}

static void dwarf_current_child(struct cc1_dbg_ctx *dbg, struct DIE *die)
{
	dwarf_child(dbg->current_scope, die);
}

void out_dbg_emit_decl(out_ctx *octx, decl *d, const out_val *val)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);

	dwarf_current_child(dbg, dbg_create_decl_die(dbg->compile_unit, d, val));
}

void out_dbg_emit_type(out_ctx *octx, type *ty)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);

	(void)dwarf_type_die(dbg->compile_unit, NULL, ty);
}

void out_dbg_emit_args_done(out_ctx *octx, funcargs *args)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);
	struct DIE *va = dwarf_maybe_create_variadic_tag(args);

	if(va)
		dwarf_current_child(dbg, va);
}

static int dw_lang_from_c_std(enum c_std std)
{
	switch(std){
		case STD_C90:
		case STD_C89: return DW_LANG_C89;
		case STD_C99: return DW_LANG_C99;
		case STD_C11: return DW_LANG_C11;
	}
	abort();
}

static struct DIE_compile_unit *dwarf_cu(
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist,
		enum c_std lang)
{
	struct DIE_compile_unit *cu = umalloc(sizeof *cu);
	form_data_t attrv;

	cu->pfilelist = pfilelist;

	dwarf_die_new_at(&cu->die, DW_TAG_compile_unit);

	dwarf_attr(&cu->die, DW_AT_producer, DW_FORM_string,
			"ucc development version");

	dwarf_attr(&cu->die, DW_AT_language, DW_FORM_data2,
			((attrv = dw_lang_from_c_std(lang)), &attrv));

	dwarf_attr(&cu->die, DW_AT_name, DW_FORM_string, (char *)fname);

	dwarf_attr(&cu->die, DW_AT_comp_dir, DW_FORM_string, (char *)compdir);

	dwarf_attr(&cu->die, DW_AT_stmt_list,
			DW_FORM_ADDR4,
			DWARF_INDIRECT_SECTION_LINKS
				? NULL
				: ustrprintf(
						"%s%s",
						SECTION_BEGIN,
						sections[SECTION_DBG_LINE].desc));

	dwarf_attr(&cu->die, DW_AT_low_pc, DW_FORM_addr,
			ustrprintf("%s%s", SECTION_BEGIN,
				sections[SECTION_TEXT].desc));

	dwarf_attr(&cu->die, DW_AT_high_pc, DW_FORM_addr,
			ustrprintf("%s%s", SECTION_END,
				sections[SECTION_TEXT].desc));

	return cu;
}

static long dwarf_info_header(FILE *f)
{
#if DWARF_INDIRECT_SECTION_LINKS
#  define VAR_LEN ASM_PLBL_PRE "info_len"
#  define VAR_OFF ASM_PLBL_PRE "abrv_off"

	fprintf(f,
			/* -4: don't include the length spec itself */
			VAR_LEN " = %s%s - %s%s - 4\n"
			VAR_OFF " = %s%s - %s%s\n"
			"\t.long " VAR_LEN "\n"
			"\t.short 2 # DWARF 2\n"
			"\t.long " VAR_OFF "  # abbrev offset\n"
			"\t.byte %d  # sizeof(void *)\n",
			SECTION_END, sections[SECTION_DBG_INFO].desc,
			SECTION_BEGIN, sections[SECTION_DBG_INFO].desc,
			SECTION_BEGIN, sections[SECTION_DBG_ABBREV].desc,
			SECTION_BEGIN, sections[SECTION_DBG_ABBREV].desc,
			platform_word_size());
#else
	fprintf(f,
			"\t.long %s%s - %s%s - 4\n"
			"\t.short 2 # DWARF 2\n"
			"\t.long %s%s  # abbrev offset\n"
			"\t.byte %d  # sizeof(void *)\n",
			SECTION_END, sections[SECTION_DBG_INFO].desc,
			SECTION_BEGIN, sections[SECTION_DBG_INFO].desc,
			SECTION_BEGIN, sections[SECTION_DBG_ABBREV].desc,
			platform_word_size());
#endif

	return 4 + 2 + 4 + 1;
}

static void dwarf_attr_decl(
		struct DIE_compile_unit *cu,
		struct DIE *in,
		decl *d,
		type *ty, int show_extern)
{
	form_data_t attrv;

	if(d->spel)
		dwarf_attr(in, DW_AT_name, DW_FORM_string, d->spel);

	dwarf_set_DW_AT_type(in, cu, NULL, ty);

	dwarf_attr(in, DW_AT_decl_file,
			DW_FORM_ULEB,
			((attrv = dbg_add_file(cu->pfilelist, d->where.fname)), &attrv));

	dwarf_attr(in, DW_AT_decl_line,
			DW_FORM_ULEB, ((attrv = d->where.line), &attrv));

	if(show_extern){
		attrv = (d->store & STORE_MASK_STORE) != store_static;
		dwarf_attr(in, DW_AT_external, DW_FORM_flag, &attrv);
	}
}

static void dwarf_location_addr(struct dwarf_block_ent *locn_ents, decl *d)
{
	locn_ents[0].type = BLOCK_HEADER;
	locn_ents[0].bits.v = DW_OP_addr;

	locn_ents[1].type = BLOCK_ADDR_STR;
	locn_ents[1].bits.str = ustrdup(decl_asm_spel(d));
}

static struct DIE *dwarf_global_variable(
		struct cc1_dbg_ctx *dbg, decl *d, const int only_if_init)
{
	struct DIE_compile_unit *cu = dbg->compile_unit;
	const enum decl_storage store = d->store & STORE_MASK_STORE;
	const int is_tdef = store == store_typedef;

	struct DIE *vardie;

	if(!d->spel)
		return NULL;

	if(only_if_init
	&& (store == store_extern || store == store_default)
	&& !d->bits.var.init.dinit)
	{
		return NULL;
	}

	vardie = dwarf_die_new(is_tdef ? DW_TAG_typedef : DW_TAG_variable);

	dwarf_attr_decl(cu, vardie, d, d->ref, !is_tdef);

	/* typedefs don't exist in the file, or have extern properties */
	if(!is_tdef){
		struct dwarf_block *locn;
		struct dwarf_block_ent *locn_ents;

		locn = umalloc(sizeof *locn);
		locn_ents = umalloc(2 * sizeof *locn_ents);

		dwarf_location_addr(locn_ents, d);

		locn->cnt = 2;
		locn->ents = locn_ents;

		dwarf_attr(vardie, DW_AT_location, DW_FORM_block1, locn);
	}

	return vardie;
}

static int should_emit_dbg_scope(symtable *stab)
{
	/* if we're at func-root, don't need to emit a lexical block */
	return stab->parent != symtab_func_root(stab);
}

void out_dbg_scope_enter(out_ctx *octx, symtable *symtab)
{
	struct cc1_dbg_ctx *dbg;
	struct DIE *scope_parent;
	struct DIE *lexblk;

	if(!should_emit_dbg_scope(symtab))
		return;

	dbg = octx2dbg(octx);
	scope_parent = dbg->current_scope;

	lexblk = dwarf_die_new(DW_TAG_lexical_block);

	dwarf_attr(lexblk, DW_AT_low_pc, DW_FORM_addr_lbl, symtab->lbl_begin);

	dwarf_attr(lexblk, DW_AT_high_pc, DW_FORM_addr_lbl, symtab->lbl_end);

	dwarf_child(scope_parent, lexblk);

	dbg->current_scope = lexblk;
}

static void dbg_scope_leave(out_ctx *octx)
{
	struct cc1_dbg_ctx *dbg;

	dbg = octx2dbg(octx);
	dbg->current_scope = dbg->current_scope->parent;
}

void out_dbg_scope_leave(out_ctx *octx, symtable *symtab)
{
	if(!should_emit_dbg_scope(symtab))
		return;

	dbg_scope_leave(octx);
}

static struct DIE *dwarf_subprogram_func(
		struct cc1_dbg_ctx *dbg, decl *d, int is_inline_instance)
{
	struct DIE_compile_unit *cu = dbg->compile_unit;
	struct DIE *subprog;
	/* generate the DW_TAG_subprogram */
	const char *asmsp;
	int code_emitted;

	subprog = dwarf_global_lookup(cu, d->spel);
	if(subprog)
		return subprog;

	code_emitted = decl_should_emit_code(d);
	if(!is_inline_instance && !code_emitted)
		return NULL;

	subprog = dwarf_die_new(DW_TAG_subprogram);

	asmsp = decl_asm_spel(d);

	dwarf_attr_decl(cu, subprog,
			d, type_func_call(d->ref, NULL),
			/*show_extern:*/1);

	if(code_emitted){
		dwarf_attr(subprog, DW_AT_low_pc, DW_FORM_addr, ustrdup(asmsp));
		dwarf_attr(subprog, DW_AT_high_pc, DW_FORM_addr, out_dbg_func_end(asmsp));
	}

	/* code gen runs through and adds decls to scope */

	return subprog;
}

struct DIE_flush
{
	struct DIE_flush_file
	{
		FILE *f;
		unsigned long byte_cnt;
	} abbrev, info;
};

enum flush_type
{
	BYTE = 1,
	WORD = 2,
	LONG = 4,
	QUAD = 8
};

static void ucc_printflike(3, 4)
	dwarf_printf(
		struct DIE_flush_file *f,
		enum flush_type sz,
		const char *fmt, ...)
{
	va_list l;
	const char *ty = NULL;
	switch(sz){
		case BYTE: ty = "byte"; break;
		case WORD: ty = "word"; break;
		case LONG: ty = "long"; break;
		case QUAD: ty = "quad"; break;
	}

	fprintf(f->f, "\t.%s ", ty);

	va_start(l, fmt);
	vfprintf(f->f, fmt, l);
	va_end(l);

	f->byte_cnt += sz;
}

static void dwarf_leb_printf(
		struct DIE_flush_file *f,
		unsigned long uleb, int is_sig)
{
	fprintf(f->f, "\t.byte ");
	f->byte_cnt += leb128_out(f->f, uleb, is_sig);
}

static void dwarf_flush_die_block(
		struct dwarf_block_ent *e, struct DIE_flush *state)
{
	switch(e->type){
		case BLOCK_HEADER:
			dwarf_printf(&state->info, BYTE,
					"%d # DW_FORM_block %s\n",
					(int)e->bits.v, die_op_to_str(e->bits.v));
			break;

		case BLOCK_LEB128_S:
		case BLOCK_LEB128_U:
			dwarf_leb_printf(&state->info,
					e->bits.v, e->type == BLOCK_LEB128_S);

			fprintf(state->info.f,
					" # DW_FORM_block, LEB%c 0x%lx\n",
					"US"[e->type == BLOCK_LEB128_S],
					e->bits.v);
			break;

		case BLOCK_ADDR_STR:
			dwarf_printf(&state->info, QUAD,
					"%s # DW_FORM_block, address\n",
					e->bits.str);
			break;
	}
}

static void dwarf_flush_die(
		struct DIE *die, struct DIE_flush *state);

static void dwarf_flush_die_children(
		struct DIE *die, struct DIE_flush *state)
{
	if(die->children){
		struct DIE **i;
		for(i = die->children; *i; i++)
			dwarf_flush_die(*i, state);

		dwarf_printf(&state->info, BYTE, "0 # end of children\n");
	}
}

static void dwarf_flush_die_1(
		struct DIE *die, struct DIE_flush *state)
{
	struct DIE_attr **at;

	UCC_ASSERT(die->locn == state->info.byte_cnt,
			"mismatching dwarf %s offset %ld vs %ld",
			die_tag_to_str(die->tag),
			die->locn, state->info.byte_cnt);

	dwarf_leb_printf(&state->abbrev, die->abbrev_code, 0),
		fprintf(state->abbrev.f, "  # Abbrev. Code %lu\n",
				die->abbrev_code);

	dwarf_leb_printf(&state->info, die->abbrev_code, 0),
		fprintf(state->info.f, "  # Abbrev. Code %lu %s\n",
				die->abbrev_code, die_tag_to_str(die->tag));

	/* tags are technically ULEBs */
	dwarf_printf(&state->abbrev, BYTE, "%d  # %s\n",
			die->tag, die_tag_to_str(die->tag));

	dwarf_printf(&state->abbrev, BYTE, "%d  # DW_CHILDREN_%s\n",
			!!die->children, die->children ? "yes" : "no");


	for(at = die->attrs; at && *at; at++){
		struct DIE_attr *a = *at;
		const char *s_attr = die_attr_to_str(a->attr);
		const char *s_enc = die_enc_to_str(a->enc);
		enum dwarf_attr_encoding enc = a->enc;

		/* synthetic encoding filter */
		switch(enc){
			default:
				break;

			case DW_FORM_ADDR4:
				enc = DW_FORM_data4;
				break;

			case DW_FORM_addr_lbl:
				if(!out_dbg_label_emitted(a->bits.lbl, NULL))
					continue;
				enc = DW_FORM_addr;
				break;
		}

		dwarf_printf(&state->abbrev, BYTE, "%d  # %s\n",
				a->attr, s_attr);

		dwarf_printf(&state->abbrev, BYTE, "%d  # %s\n",
				enc, s_enc);

		switch(a->enc){
				enum flush_type fty;
			case DW_FORM_data1: fty = BYTE; goto form_data;
			case DW_FORM_data2: fty = WORD; goto form_data;
			case DW_FORM_data4: fty = LONG; goto form_data;
			case DW_FORM_data8: fty = QUAD; goto form_data;
form_data:
				dwarf_printf(&state->info, fty, "%ld", a->bits.value);
				break;

			case DW_FORM_ULEB:
				dwarf_leb_printf(&state->info, a->bits.value, 0);
				fputc('\n', state->info.f);
				break;

			case DW_FORM_ADDR4: fty = LONG; goto addr;
			case DW_FORM_addr: fty = QUAD; goto addr;
addr:
				dwarf_printf(&state->info, fty, "%s",
						a->bits.str ?  a->bits.str : "0");
				break;

			case DW_FORM_addr_lbl:
			{
				const char *lbl;
				int emit = out_dbg_label_emitted(a->bits.lbl, &lbl);
				assert(emit);
				dwarf_printf(&state->info, QUAD, "%s", lbl);
				break;
			}

			case DW_FORM_string:
				fprintf(state->info.f, "\t.ascii \"%s\"\n", a->bits.str);
				state->info.byte_cnt += strlen(a->bits.str);

				dwarf_printf(&state->info, BYTE, "0");
				break;

			case DW_FORM_ref4:
				UCC_ASSERT(a->bits.die_xref->locn,
						"unset DIE/%s location",
						die_tag_to_str(a->bits.die_xref->tag));
				UCC_ASSERT(a->bits.die_xref->locn != die->locn,
						"subDIE has same location");

				dwarf_printf(&state->info, LONG, "%lu", a->bits.die_xref->locn);
				break;
			case DW_FORM_flag:
				dwarf_printf(&state->info, BYTE, "%d", (int)a->bits.value);
				break;
			case DW_FORM_block1:
			{
				int i;
				unsigned len = 0;

				for(i = 0; i < a->bits.blk->cnt; i++){
					struct dwarf_block_ent *e = &a->bits.blk->ents[i];

					switch(e->type){
						case BLOCK_HEADER:
							len++;
							break;
						case BLOCK_LEB128_S:
						case BLOCK_LEB128_U:
							len += leb128_length(e->bits.v,
									e->type == BLOCK_LEB128_S);
							break;
						case BLOCK_ADDR_STR:
							len += platform_word_size();
							break;
					}
				}

				UCC_ASSERT(len > 0, "zero length block, count %d", a->bits.blk->cnt);
				dwarf_printf(&state->info, BYTE, "%d # block count\n",
						len);

				for(i = 0; i < a->bits.blk->cnt; i++)
					dwarf_flush_die_block(
							&a->bits.blk->ents[i],
							state);
				break;
			}
		}
		fprintf(state->info.f, " # %s\n", s_attr);
	}

	fprintf(state->abbrev.f,
			"\t.byte 0, 0 # name/val abbrev %lu end\n\n",
			die->abbrev_code);
	state->abbrev.byte_cnt += 2;

	fprintf(state->info.f, "\n");
}

static void dwarf_flush_die(
		struct DIE *die, struct DIE_flush *state)
{
	dwarf_flush_die_1(die, state);
	dwarf_flush_die_children(die, state);
}

static void dwarf_flush(struct DIE_compile_unit *cu,
		FILE *abbrev, FILE *info, long initial_offset)
{
	struct DIE_flush flush = {{ 0 }};

	flush.info.byte_cnt = initial_offset;
	flush.info.f = info;
	flush.abbrev.f = abbrev;

	dwarf_flush_die(&cu->die, &flush);

	fprintf(abbrev, "\t.byte 0 # end\n");
}

static unsigned long dwarf_offset_die(
		struct DIE *die,
		unsigned long *abbrev, unsigned long off)
{
	struct DIE_attr **at;

	UCC_ASSERT(die->locn == 0, "double offset die?");

	die->locn = off;

	die->abbrev_code = ++*abbrev;
	off += leb128_length(die->abbrev_code, 0);

	for(at = die->attrs; at && *at; at++){
		struct DIE_attr *a = *at;

		enum dwarf_attr_encoding enc = a->enc;

		switch(enc){
			case DW_FORM_addr_lbl:
				if(!out_dbg_label_emitted(a->bits.lbl, NULL))
					break;
				/* fall */

			case DW_FORM_addr:  off += QUAD; break;
			case DW_FORM_ADDR4: off += LONG; break;

			case DW_FORM_data1: off += BYTE; break;
			case DW_FORM_data2: off += WORD; break;
			case DW_FORM_data4: off += LONG; break;
			case DW_FORM_data8: off += QUAD; break;

			case DW_FORM_ULEB:
				off += leb128_length(a->bits.value, 0);
				break;

			case DW_FORM_string:
				off += strlen(a->bits.str) + 1;
				break;

			case DW_FORM_ref4:
				off += LONG;
				break;
			case DW_FORM_flag:
				off++;
				break;
			case DW_FORM_block1:
			{
				int i;

				off++; /* block count byte */

				for(i = 0; i < a->bits.blk->cnt; i++){
					struct dwarf_block_ent *e = &a->bits.blk->ents[i];

					switch(e->type){
						case BLOCK_HEADER:
							off++;
							break;
						case BLOCK_LEB128_S:
						case BLOCK_LEB128_U:
							off += leb128_length(e->bits.v,
									e->type == BLOCK_LEB128_S);
							break;
						case BLOCK_ADDR_STR:
							off += QUAD;
							break;
					}
				}
				break;
			}
		}
	}

	if(die->children){
		struct DIE **i;
		for(i = die->children; *i; i++)
			off = dwarf_offset_die(*i, abbrev, off);

		off++; /* end of children mark */
	}

	return off;
}

static void dbg_emit_global(out_ctx *octx, struct DIE *global)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);

	if(global)
		dwarf_child(&dbg->compile_unit->die, global);
}

void out_dbg_emit_global_decl_scoped(out_ctx *octx, decl *d)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);
	struct DIE *global_scoped = dwarf_global_variable(dbg, d, 0);

	if(global_scoped)
		dwarf_current_child(dbg, global_scoped);
}

void dbg_out_filelist(
		struct out_dbg_filelist *head, FILE *f)
{
	struct out_dbg_filelist *i;
	unsigned idx;

	for(i = head, idx = 1; i; i = i->next, idx++){
		char *esc = str_add_escape(i->fname, strlen(i->fname));

		fprintf(f, ".file %u \"%s\"\n", idx, esc);
		free(esc);
	}
}

void out_dbg_begin(
		out_ctx *octx,
		struct out_dbg_filelist **pfilelist,
		const char *fname,
		const char *compdir,
		enum c_std lang)
{
	struct DIE_compile_unit *cu = dwarf_cu(fname, compdir, pfilelist, lang);
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);

	dbg->compile_unit = cu;
	dbg->current_scope = NULL;
}

void out_dbg_end(out_ctx *octx)
{
	long info_offset = dwarf_info_header(cc_out[SECTION_DBG_INFO]);
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);
	struct DIE_compile_unit *compile_unit = dbg->compile_unit;
	unsigned long abbrev = 0;
	size_t i;
	struct DIE *tydie;

	/* current_scope should be a direct child of the compile unit */
	if(dbg->current_scope && dbg->current_scope->parent != &compile_unit->die)
		ICW("debug still has current_scope");

	for(i = 0;
	    (tydie = dynmap_value(struct DIE *, compile_unit->types_to_dies, i));
	    i++)
	{
		dwarf_child(&compile_unit->die, tydie);
	}

	dwarf_offset_die(&compile_unit->die, &abbrev, info_offset);

	dwarf_flush(compile_unit,
			cc_out[SECTION_DBG_ABBREV],
			cc_out[SECTION_DBG_INFO],
			info_offset);

	/* no need to dwarf_die_free_1() type dies - they're children of the CU */
	dynmap_free(compile_unit->types_to_dies);

	dwarf_die_free_r(&compile_unit->die);
}

void out_dbg_emit_func(out_ctx *octx, decl *d)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);
	struct DIE *subprog;

	assert(type_is_func_or_block(d->ref));

	subprog = dwarf_subprogram_func(dbg, d, 0);
	dbg_emit_global(octx, subprog);

	dbg->current_scope = subprog;
}

void out_dbg_emit_global_var(out_ctx *octx, decl *d)
{
	dbg_emit_global(octx, dwarf_global_variable(octx2dbg(octx), d, 1));
}

void out_dbg_inlined_call(
		out_ctx *octx,
		decl *dinlined,
		struct out_dbg_lbl *caller_start_lbl,
		struct out_dbg_lbl *caller_end_lbl,
		const where *call_loc)
{
	struct cc1_dbg_ctx *dbg = octx2dbg(octx);
	struct DIE_compile_unit *cu = dbg->compile_unit;
	struct DIE *tag = dwarf_die_new(DW_TAG_inlined_subroutine);
	struct DIE *lookup_fn = dwarf_global_lookup(cu, dinlined->spel);
	form_data_t form_data;

	if(!lookup_fn){
		/* pure inline function, or later defined */
		lookup_fn = dwarf_subprogram_func(dbg, dinlined, 1);

		dwarf_child(&cu->die, lookup_fn);
	}

	dwarf_attr(tag, DW_AT_abstract_origin, DW_FORM_ref4, lookup_fn);

	/* mark the origin as inlined */
	if(!dwarf_attr_lookup(lookup_fn, DW_AT_inline)){
		form_data_t flag = DW_INL_inlined;
		dwarf_attr(lookup_fn, DW_AT_inline, DW_FORM_data1, &flag);
	}

	dwarf_attr(tag, DW_AT_low_pc, DW_FORM_addr_lbl, caller_start_lbl);
	dwarf_attr(tag, DW_AT_high_pc, DW_FORM_addr_lbl, caller_end_lbl);

	form_data = dbg_add_file(cu->pfilelist, call_loc->fname);
	dwarf_attr(tag, DW_AT_call_file, DW_FORM_ULEB, &form_data);

	form_data = call_loc->line;
	dwarf_attr(tag, DW_AT_call_line, DW_FORM_ULEB, &form_data);

	dwarf_current_child(dbg, tag);

	dbg->current_scope = tag;
}

void out_dbg_inline_end(out_ctx *octx)
{
	/* pop the scope that the DW_TAG_inlined_subroutine added */
	dbg_scope_leave(octx);
}
