#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../../util/where.h"
#include "../../util/platform.h"
#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/dynmap.h"
#include "../../util/alloc.h"

#include "../str.h"

#include "../data_structs.h"
#include "../expr.h"
#include "../tree.h"
#include "../const.h"
#include "../funcargs.h"
#include "../sue.h"

#include "asm.h" /* cc_out[] */

#include "../defs.h" /* CHAR_BIT */

#include "lbl.h"
#include "dbg.h"

struct dwarf_state
{
	struct dwarf_sec
	{
		struct dwarf_val
		{
			enum
			{
				DWARF_BYTE = 1,
				DWARF_WORD = 2,
				DWARF_LONG = 4,
				DWARF_QUAD = 8,
				DWARF_STR  = 9,
				DWARF_SIBLING = 10,
				DWARF_ADDR_STR = 11,
				DWARF_HELP = 12,
				DWARF_LEB128 = 13,
			} val_type;
			union
			{
				unsigned long long value;
				char *str; /* FREE */
				char *addr_str; /* ref to d->spel */
				unsigned sibling_pos; /* filled in later */
				char *help; /* FREE */
			} bits;

			unsigned sibling_nest;
			int indent_adj;
		} *values; /* FREE */
		size_t nvalues;
		size_t length;

		int last_idx;
		unsigned current_sibling_nest;
	} abbrev, info;
#define DWARF_SEC_INIT() { NULL, 0, 0, 1, 0 }

	dynmap *type_ref_to_off; /* type_ref * => unsigned * */
};

struct dwarf_block /* DW_FORM_block1 */
{
	unsigned cnt; /* length is the sum of entry sizes */
	struct dwarf_block_ent
	{
		enum
		{
			BLOCK_N,
			BLOCK_ADDR_STR,
			BLOCK_LEB128
		} type;
		union
		{
			unsigned n;
			char *addr_str;
		} bits;
	} *vals;
};

enum dwarf_block_ops
{
	DW_OP_plus_uconst = 0x23,
	DW_OP_addr = 0x3
};

enum dwarf_key
{
	DW_TAG_base_type = 0x24,
	DW_TAG_variable = 0x34,
	DW_TAG_typedef = 0x16,
	DW_TAG_pointer_type = 0xf,
	DW_TAG_array_type = 0x1,
	DW_TAG_subrange_type = 0x21,
	DW_TAG_const_type = 0x26,
	DW_TAG_subroutine_type = 0x15,
	DW_TAG_formal_parameter = 0x5,
	DW_TAG_enumeration_type = 0x4,
	DW_TAG_enumerator = 0x28,
	DW_TAG_structure_type = 0x13,
	DW_TAG_union_type = 0x17,
	DW_TAG_member = 0xd,
	DW_AT_data_member_location = 0x38,
	DW_AT_external = 0x3f,

	DW_AT_byte_size = 0xb,
	DW_AT_encoding = 0x3e,
	DW_AT_bit_offset = 0xc,
	DW_AT_bit_size = 0xd,

	DW_AT_name = 0x3,
	DW_AT_language = 0x13,
	DW_AT_low_pc = 0x11,
	DW_AT_high_pc = 0x12,
	DW_AT_producer = 0x25,
	DW_AT_type = 0x49,
	DW_AT_sibling = 0x1,
	DW_AT_lower_bound = 0x22,
	DW_AT_upper_bound = 0x2f,
	DW_AT_prototyped = 0x27,
	DW_AT_location = 0x2,
	DW_AT_const_value = 0x1c,
	DW_AT_accessibility = 0x32,
	DW_ACCESS_public = 0x01,
};
enum dwarf_valty
{
	DW_FORM_addr = 0x1,
	DW_FORM_data1 = 0xb,
	DW_FORM_data2 = 0x5,
	DW_FORM_data4 = 0x6,
	/*DW_FORM_data8 = 0x7,*/
	DW_FORM_dataN = 0x9, /* use data4 as a no-clobber placeholder */
	DW_FORM_string = 0x8,
	/*
	DW_FORM_ref1 = 0x11,
	DW_FORM_ref2 = 0x12,
	DW_FORM_ref8 = 0x14,
	*/
	DW_FORM_ref4 = 0x13,
	DW_FORM_flag = 0xc,
	DW_FORM_block1 = 0xa,
};
enum
{
	DW_TAG_compile_unit = 0x11,
	DW_CHILDREN_no = 0,
	DW_CHILDREN_yes = 1,

	DW_ATE_boolean = 0x2,
	DW_ATE_float = 0x4,
	DW_ATE_signed = 0x5,
	DW_ATE_signed_char = 0x6,
	DW_ATE_unsigned = 0x7,
	DW_ATE_unsigned_char = 0x8,

	DW_LANG_C89 = 0x1,
	DW_LANG_C99 = 0xc
};

static void dwarf_attr(
		struct dwarf_state *st,
		enum dwarf_key key, enum dwarf_valty val,
		...);

static void dwarf_smallest(
		unsigned long val, unsigned *int_sz)
{
	if((unsigned char)val == val)
		*int_sz = 1;
	else if((unsigned short)val == val)
		*int_sz = 2;
	else if((unsigned)val == val)
		*int_sz = 4;
	else
		*int_sz = 8;
}

static struct dwarf_val *dwarf_value_new(struct dwarf_sec *sec)
{
	struct dwarf_val *val;

	sec->values = urealloc1(sec->values, ++sec->nvalues * sizeof *sec->values);

	val = &sec->values[sec->nvalues-1];
	memset(val, 0, sizeof *val);
	return val;
}

static void dwarf_help(
		struct dwarf_state *st,
		const char *fmt, ...)
{
	va_list l;
	char buf[256];
	struct dwarf_val *val;

	va_start(l, fmt);
	vsnprintf(buf, sizeof buf, fmt, l);
	va_end(l);

	val = dwarf_value_new(&st->abbrev);
	val->val_type = DWARF_HELP;
	val->bits.help = ustrdup(buf);
}

static void dwarf_add_value(
		struct dwarf_sec *sec,
		unsigned val_type,
		unsigned long long value)
{
	struct dwarf_val *val = dwarf_value_new(sec);

	val->val_type = val_type;
	val->bits.value = value;

	sec->length += val_type;
}

static unsigned dwarf_leb128_length(unsigned long long value)
{
	unsigned len;
	for(len = 0;;){
		value >>= 7;

		len++;
		if(value) /* more */
			continue;
		break;
	}
	return len;
}

static void dwarf_add_leb128(
		struct dwarf_sec *sec, unsigned long long value)
{
	struct dwarf_val *val = dwarf_value_new(sec);

	val->val_type = DWARF_LEB128;
	val->bits.value = value;

	sec->length += dwarf_leb128_length(value);
}

static void dwarf_add_str(struct dwarf_sec *sec, char *str)
{
	struct dwarf_val *val = dwarf_value_new(sec);

	val->val_type = DWARF_STR;
	val->bits.str = str;

	sec->length += strlen(str) + 1;
}

static void dwarf_add_addr_str(
		struct dwarf_sec *sec, char *addr)
{
	struct dwarf_val *val = dwarf_value_new(sec);

	val->val_type = DWARF_ADDR_STR;
	val->bits.addr_str = addr;

	sec->length += platform_word_size();
}

static void dwarf_sec_indent(struct dwarf_sec *sec, int chg)
{
	sec->values[sec->nvalues-1].indent_adj = chg;
}

static void dwarf_sibling_push(struct dwarf_state *st)
{
	/* ref4 TBA */
	dwarf_attr(st, DW_AT_sibling, DW_FORM_ref4, 0);

	{
		struct dwarf_sec *sec = &st->info;
		struct dwarf_val *val = &sec->values[sec->nvalues-1];

		/* val is the DW_FORM_ref4 entry in the info section */

		val->val_type = DWARF_SIBLING; /* will have been DWARF_LONG */
		val->bits.sibling_pos = 0; /* to update */
		val->sibling_nest = ++sec->current_sibling_nest;
	}
}

static void dwarf_sibling_pop(struct dwarf_state *st)
{
	struct dwarf_sec *sec = &st->info;
	/* work our way back through siblings until we find one
	 * at our nest level, and update its sibling pointer */
	size_t i;

	for(i = sec->nvalues - 1;; i--){
		struct dwarf_val *val = &sec->values[i];

		if(val->sibling_nest == sec->current_sibling_nest){
			/* found */

			/* restore sibling */
			sec->current_sibling_nest--;

			/* update previous sibling offset */
			val->bits.sibling_pos = sec->length + 1 /* include the child mark */;
			break;
		}

		if(i == 0)
			ICE("couldn't find sibling entry for dwarf node");
	}

	/* end of child mark */
	dwarf_add_value(sec, /*byte:*/1, /*nul*/0);
}

#define VAL_TERM -1L
static void dwarf_sec_val(struct dwarf_sec *sec, long val, ...) /* -1 terminator */
{
	va_list l;

	va_start(l, val);

	do{
		unsigned int_sz;

		dwarf_smallest(val, &int_sz);

		dwarf_add_value(sec, int_sz, val);

		val = va_arg(l, long);
	}while(val != VAL_TERM);

	va_end(l);
}

static void dwarf_attr(
		struct dwarf_state *st,
		enum dwarf_key key, enum dwarf_valty val,
		...)
{
	va_list l;

	/* abbrev part */
	dwarf_sec_val(&st->abbrev, key, val, VAL_TERM);

	/* info part */
	va_start(l, val);
	switch(val){
		case DW_FORM_block1:
		{
			const struct dwarf_block *blk = va_arg(l, struct dwarf_block *);
			size_t i;
			unsigned long len = 0;
			unsigned sz;

			for(i = 0; i < blk->cnt; i++)
				switch(blk->vals[i].type){
					case BLOCK_LEB128:
						len += dwarf_leb128_length(blk->vals[i].bits.n);
						break;
					case BLOCK_N:
						dwarf_smallest(blk->vals[i].bits.n, &sz);
						len += sz;
						break;
					case BLOCK_ADDR_STR:
						len += platform_word_size();
						break;
				}

			dwarf_smallest(len, &sz);
			dwarf_add_value(&st->info, sz, len);

			for(i = 0; i < blk->cnt; i++){
				switch(blk->vals[i].type){
					case BLOCK_LEB128:
						dwarf_add_leb128(&st->info, blk->vals[i].bits.n);
						break;
					case BLOCK_N:
					{
						unsigned long val = blk->vals[i].bits.n;
						unsigned sz;
						dwarf_smallest(val, &sz);
						dwarf_add_value(&st->info, sz, val);
						break;
					}
					case BLOCK_ADDR_STR:
						dwarf_add_addr_str(&st->info, blk->vals[i].bits.addr_str);
						break;
				}
			}
			break;
		}
		case DW_FORM_ref4:
			dwarf_add_value(&st->info, /*long:*/4, va_arg(l, unsigned));
			break;
		case DW_FORM_addr:
			dwarf_add_value(&st->info, /*quad:*/8, (long)va_arg(l, void *));
			break;
		{
			unsigned sz;
		case DW_FORM_flag: sz = 1; goto add_data;
		case DW_FORM_data1: sz = 1; goto add_data;
		case DW_FORM_data2: sz = 2; goto add_data;
		case DW_FORM_data4: sz = 4; goto add_data;
add_data:
			dwarf_add_value(&st->info, sz, va_arg(l, int));
			break;
		}
		case DW_FORM_dataN:
		{
			long long val = va_arg(l, long long);
			unsigned sz;
			dwarf_smallest(val, &sz);
			dwarf_add_value(&st->info, sz, val);
			break;
		}
		case DW_FORM_string:
		{
			char *esc = va_arg(l, char *);
			esc = str_add_escape(esc, strlen(esc));
			dwarf_add_str(&st->info, esc);
			break;
		}
	}
	va_end(l);
}

static void dwarf_sec_start(struct dwarf_sec *sec)
{
	dwarf_sec_val(sec, sec->last_idx++, VAL_TERM);
	dwarf_sec_indent(sec, +1);
}

static void dwarf_sec_end(struct dwarf_sec *sec)
{
	dwarf_sec_val(sec, 0, VAL_TERM);
	dwarf_sec_indent(sec, -1);
}

static void dwarf_start(struct dwarf_state *st)
{
	dwarf_sec_start(&st->abbrev);
	dwarf_sec_start(&st->info);
}

static void dwarf_end(struct dwarf_state *st)
{
	dwarf_sec_end(&st->abbrev);
	dwarf_sec_indent(&st->info, -1); /* we don't terminate info entries */
}

static void dwarf_abbrev_start(struct dwarf_state *st, int b1, int b2)
{
	dwarf_sec_val(&st->abbrev, b1, b2, VAL_TERM);
	dwarf_sec_indent(&st->abbrev, +1);
}

static void dwarf_basetype(struct dwarf_state *st, enum type_primitive prim)
{
	int enc;
	switch(prim){
		case type__Bool:
			enc = DW_ATE_boolean;
			break;

		case type_nchar:
		case type_schar:
			enc = DW_ATE_signed_char;
			break;
		case type_uchar:
			enc = DW_ATE_unsigned_char;
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

	dwarf_help(st, "base type %s", type_primitive_to_str(prim));

	dwarf_start(st); {
		dwarf_abbrev_start(st, DW_TAG_base_type, DW_CHILDREN_no); {
			dwarf_attr(st, DW_AT_name,      DW_FORM_string, type_primitive_to_str(prim));
			dwarf_attr(st, DW_AT_byte_size, DW_FORM_data4,  type_primitive_size(prim));
			dwarf_attr(st, DW_AT_encoding,  DW_FORM_data1,  enc);
		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static void dwarf_sue_header(
		struct dwarf_state *st, struct_union_enum_st *sue,
		int dwarf_tag)
{
	dwarf_start(st); {
		dwarf_abbrev_start(st, dwarf_tag, DW_CHILDREN_yes); {
			dwarf_sibling_push(st);
			if(!sue->anon)
				dwarf_attr(st, DW_AT_name, DW_FORM_string, sue->spel);
			if(sue_complete(sue))
				dwarf_attr(st, DW_AT_byte_size, DW_FORM_data4, (long long)sue_size(sue, NULL));
		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static unsigned dwarf_type(struct dwarf_state *st, type_ref *ty)
{
	unsigned this_start;

	unsigned *map_ent = dynmap_get(type_ref *, unsigned *, st->type_ref_to_off, ty);
	if(map_ent)
		return *map_ent;

	dwarf_help(st, "%s", type_ref_to_str(ty));

	switch(ty->type){
		case type_ref_type:
		{
			struct_union_enum_st *sue = ty->bits.type->sue;

			this_start = st->info.length;

			if(sue){
				switch(sue->primitive){
					default:
						ucc_unreach(0);

					case type_enum:
					{
						sue_member **i;

						dwarf_help(st, "%s header", sue->spel);
						dwarf_sue_header(st, sue, DW_TAG_enumeration_type);

						/* enumerators */
						for(i = sue->members; i && *i; i++){
							enum_member *emem = (*i)->enum_member;

							dwarf_help(st, "  %s::%s", sue->spel, emem->spel);

							dwarf_start(st); {
								dwarf_abbrev_start(st, DW_TAG_enumerator, DW_CHILDREN_no); {
									dwarf_attr(st,
											DW_AT_name, DW_FORM_string,
											emem->spel);

									dwarf_attr(st,
											DW_AT_const_value, DW_FORM_data4,
											(long long)const_fold_val_i(emem->val));

								} dwarf_sec_end(&st->abbrev);
							} dwarf_end(st);
						}

						dwarf_sibling_pop(st);
						break;
					}

					case type_union:
					case type_struct:
					{
						const size_t nmem = dynarray_count(sue->members);
						sue_member **si;
						unsigned i;
						unsigned *mem_offsets = nmem ? umalloc(nmem * sizeof *mem_offsets) : NULL;

						/* member types */
						for(i = 0; i < nmem; i++)
							mem_offsets[i] = dwarf_type(st, sue->members[i]->struct_member->ref);

						/* must update since we might've output extra type information */
						this_start = st->info.length;

						dwarf_help(st, "%s header", sue->spel);
						dwarf_sue_header(
								st, sue,
								sue->primitive == type_struct
									? DW_TAG_structure_type
									: DW_TAG_union_type);

						/* members */
						for(i = 0, si = sue->members; i < nmem; i++, si++){
							decl *dmem = (*si)->struct_member;

							dwarf_help(st, "  %s::%s", sue->spel, decl_to_str(dmem));

							/* skip, otherwise dwarf thinks we've a field and messes up */
							if(!dmem->spel)
								continue;

							dwarf_start(st); {
								dwarf_abbrev_start(st, DW_TAG_member, DW_CHILDREN_no); {
									struct dwarf_block offset;
									struct dwarf_block_ent offset_data[2];

									dwarf_attr(st,
											DW_AT_name, DW_FORM_string,
											dmem->spel);

									dwarf_attr(st,
											DW_AT_type, DW_FORM_ref4,
											mem_offsets[i]);

									offset_data[0].type = BLOCK_N;
									offset_data[0].bits.n = DW_OP_plus_uconst;
									offset_data[1].type = BLOCK_LEB128;
									offset_data[1].bits.n = dmem->struct_offset;

									offset.cnt = 2;
									offset.vals = offset_data;

									dwarf_attr(st,
											DW_AT_data_member_location, DW_FORM_block1,
											&offset);

									/* bitfield */
									if(dmem->field_width){
										unsigned width = const_fold_val_i(dmem->field_width);
										unsigned whole_sz = type_ref_size(dmem->ref, NULL);

										/* address of top-end */
										unsigned off =
											(whole_sz * CHAR_BIT)
											- (width + dmem->struct_offset_bitfield);

										dwarf_help(st, "bitfield %u-%u (sz=%u off=%u)",
												width + dmem->struct_offset_bitfield,
												dmem->struct_offset_bitfield,
												off, width);

										dwarf_attr(st,
												DW_AT_bit_offset, DW_FORM_data1,
												off);

										dwarf_attr(st,
												DW_AT_bit_size, DW_FORM_data1,
												width);
									}

									dwarf_attr(st,
											DW_AT_accessibility, DW_FORM_flag,
											DW_ACCESS_public);

								} dwarf_sec_end(&st->abbrev);
							} dwarf_end(st);
						}

						dwarf_sibling_pop(st);
						free(mem_offsets);
						break;
					}
				}

			}else{
				dwarf_basetype(st, ty->bits.type->primitive);
			}
			break;
		}

		case type_ref_tdef:
			if(ty->bits.tdef.decl){
				decl *d = ty->bits.tdef.decl;
				const unsigned sub_pos = dwarf_type(st, d->ref);

				this_start = st->info.length;

				dwarf_start(st); {
					dwarf_abbrev_start(st, DW_TAG_typedef, DW_CHILDREN_no); {
						dwarf_attr(st, DW_AT_name, DW_FORM_string, d->spel);
						dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
					} dwarf_sec_end(&st->abbrev);
				} dwarf_end(st);
			}else{
				/* skip typeof() */
				this_start = st->info.length;
				dwarf_type(st, ty->bits.tdef.type_of->tree_type);
			}
			break;

		case type_ref_ptr:
		{
			const unsigned sub_pos = dwarf_type(st, ty->ref);

			this_start = st->info.length;

			dwarf_start(st); {
				dwarf_abbrev_start(st, DW_TAG_pointer_type, DW_CHILDREN_no); {
					dwarf_attr(st, DW_AT_byte_size, DW_FORM_data4, platform_word_size());
					dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
				} dwarf_sec_end(&st->abbrev);
			} dwarf_end(st);
			break;
		}

		case type_ref_block:
			/* skip */
			this_start = st->info.length;
			dwarf_type(st, ty->ref);
			break;

		case type_ref_func:
		{
			decl **i;
			const unsigned pos_ref = dwarf_type(st, ty->ref);
			/*pos_sibling = pos_ref + ??? */

			this_start = st->info.length;

			dwarf_start(st); {
				dwarf_abbrev_start(st, DW_TAG_subroutine_type, DW_CHILDREN_no); {
					/*dwarf_attr(st, DW_AT_sibling, DW_FORM_ref4, pos_sibling);*/
					dwarf_attr(st, DW_AT_type, DW_FORM_ref4, pos_ref);
					dwarf_attr(st, DW_AT_prototyped, DW_FORM_flag, 1);
				} dwarf_sec_end(&st->abbrev);
			} dwarf_end(st);

			for(i = ty->bits.func.args->arglist;
			    i && *i;
			    i++)
			{
				const unsigned sub_pos = dwarf_type(st, (*i)->ref);

				dwarf_start(st); {
					dwarf_abbrev_start(st, DW_TAG_formal_parameter, DW_CHILDREN_no); {
						dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
					} dwarf_sec_end(&st->abbrev);
				} dwarf_end(st);
			}
			break;
		}

		case type_ref_array:
		{
			int have_sz = !!ty->bits.array.size;
			integral_t sz;
			const unsigned sub_pos = dwarf_type(st, ty->ref);

			this_start = st->info.length;

			if(have_sz)
				sz = const_fold_val_i(ty->bits.array.size);

			dwarf_start(st); {
				dwarf_abbrev_start(st, DW_TAG_array_type, DW_CHILDREN_yes); {
					dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
					dwarf_sibling_push(st);
				} dwarf_sec_end(&st->abbrev);
			} dwarf_end(st);

			dwarf_start(st); {
				dwarf_abbrev_start(st, DW_TAG_subrange_type, DW_CHILDREN_no); {
					if(have_sz && sz > 0){
						/*dwarf_attr(st, DW_AT_lower_bound, DW_FORM_data4, 0);*/
						dwarf_attr(st, DW_AT_upper_bound, DW_FORM_data4, (long long)(sz - 1));
					}
				} dwarf_sec_end(&st->abbrev);
			} dwarf_end(st);

			dwarf_sibling_pop(st);
			break;
		}

		case type_ref_cast:
		{
			const unsigned sub_pos = dwarf_type(st, ty->ref);
			this_start = st->info.length;

			if(ty->bits.cast.is_signed_cast){
				/* skip */
			}else{
				dwarf_start(st); {
					dwarf_abbrev_start(st, DW_TAG_const_type, DW_CHILDREN_no); {
						dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
					} dwarf_sec_end(&st->abbrev);
				} dwarf_end(st);
			}
			break;
		}
	}

	map_ent = umalloc(sizeof *map_ent);
	*map_ent = this_start;
	dynmap_set(type_ref *, unsigned *, st->type_ref_to_off, ty, map_ent);

	return this_start;
}

static void dwarf_cu(struct dwarf_state *st, const char *fname)
{
	dwarf_start(st); {
		dwarf_abbrev_start(st, DW_TAG_compile_unit, DW_CHILDREN_yes); {
			dwarf_attr(st, DW_AT_producer, DW_FORM_string, "ucc development version");
			dwarf_attr(st, DW_AT_language, DW_FORM_data2, DW_LANG_C99);
			dwarf_attr(st, DW_AT_name, DW_FORM_string, fname);
			dwarf_attr(st, DW_AT_low_pc, DW_FORM_addr, 12345); /* TODO */
			dwarf_attr(st, DW_AT_high_pc, DW_FORM_addr, 54321);
		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static void dwarf_info_header(struct dwarf_sec *sec, FILE *f)
{
	/* hacky? */
	fprintf(f,
			"\t.long .Ldbg_info_end - .Ldbg_info_start\n"
			".Ldbg_info_start:\n"
			"\t.short 2 # DWARF 2\n"
			"\t.long 0  # abbrev offset\n"
			"\t.byte %d  # sizeof(void *)\n",
			platform_word_size());

	sec->length += 4 + 2 + 4 + 1;
}

static void dwarf_info_footer(struct dwarf_sec *sec, FILE *f)
{
	(void)sec;
	fprintf(f, ".Ldbg_info_end:\n");
}

static void dwarf_global_variable(struct dwarf_state *st, decl *d)
{
	enum decl_storage const store = d->store & STORE_MASK_STORE;
	unsigned typos;

	if(!d->spel)
		return;

	typos = dwarf_type(st, d->ref);

	dwarf_help(st, "global %s", decl_to_str(d));

	dwarf_start(st); {
		dwarf_abbrev_start(st,
				store == store_typedef ? DW_TAG_typedef : DW_TAG_variable,
				DW_CHILDREN_no);
		{
			dwarf_attr(st, DW_AT_name, DW_FORM_string, d->spel);
			dwarf_attr(st, DW_AT_type, DW_FORM_ref4, typos);

			/* typedefs don't exist in the file, or have extern properties */
			if(store != store_typedef){
				struct dwarf_block locn;
				struct dwarf_block_ent locn_data[2];

				locn_data[0].type = BLOCK_N;
				locn_data[0].bits.n = DW_OP_addr;
				locn_data[1].type = BLOCK_ADDR_STR;
				locn_data[1].bits.addr_str = d->spel;

				locn.cnt = 2;
				locn.vals = locn_data;
				dwarf_attr(st, DW_AT_location, DW_FORM_block1, &locn);

				dwarf_attr(st, DW_AT_external, DW_FORM_flag, store != store_static);
			}

		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static void dwarf_flush(struct dwarf_sec *sec, FILE *f)
{
	unsigned indent = 0;
	size_t i;

	for(i = 0; i < sec->nvalues; i++){
		const struct dwarf_val *val = &sec->values[i];
		unsigned indent_adj;

		/* increments take affect before, decrements, after */
		if(val->indent_adj > 0)
			indent += val->indent_adj;
		for(indent_adj = 0; indent_adj < indent; indent_adj++)
			fputc('\t', f);
		if(val->indent_adj < 0){
			if((unsigned)-val->indent_adj > indent){
				indent = 0;
			}else{
				indent += val->indent_adj;
			}
		}

		switch(val->val_type){
			{
				const char *ty;
			case DWARF_BYTE: ty = "byte"; goto o_common;
			case DWARF_WORD: ty = "word"; goto o_common;
			case DWARF_LONG: ty = "long"; goto o_common;
			case DWARF_QUAD: ty = "quad"; goto o_common;
o_common:
				fprintf(f, ".%s %lld\n", ty, val->bits.value);
				break;
			}

			case DWARF_LEB128:
			{
				unsigned long long v = val->bits.value;
				const char *join = "";

				fputs(".byte ", f);
				for(;;){
					unsigned char byte = v & 0x7f;

					v >>= 7;
					if(v){
						/* more */
						byte |= 0x80; /* 0b_1000_0000 */
					}

					fprintf(f, "%s%d", join, byte);
					join = ", ";

					if(byte & 0x80)
						continue;
					break;
				}
				fprintf(f, " # uleb128 of %llu\n", val->bits.value);
				break;
			}

			case DWARF_SIBLING:
			{
				const unsigned long pos = val->bits.sibling_pos;
				UCC_ASSERT(pos, "unset sibling offset");
				fprintf(f, ".long %lu\n", pos);
				break;
			}

			case DWARF_ADDR_STR:
				fprintf(f, ".quad %s\n", val->bits.addr_str);
				break;
			case DWARF_STR:
				fprintf(f, ".asciz \"%s\"\n", val->bits.str);
				break;

			case DWARF_HELP:
				fprintf(f, "# %s\n", val->bits.help);
				break;
		}
	}
}

static void dwarf_sec_free(struct dwarf_sec *sec)
{
	size_t i;
	for(i = 0; i < sec->nvalues; i++){
		struct dwarf_val *val = &sec->values[i];
		switch(val->val_type){
				case DWARF_BYTE:
				case DWARF_WORD:
				case DWARF_LONG:
				case DWARF_QUAD:
				case DWARF_ADDR_STR:
				case DWARF_SIBLING:
				case DWARF_LEB128:
					break;
				case DWARF_STR:
					free(val->bits.str);
					break;
				case DWARF_HELP:
					free(val->bits.help);
					break;
		}
	}
	free(sec->values);
}

static void dwarf_st_free(struct dwarf_state *st)
{
	size_t i;
	unsigned *pos;

	for(i = 0;
	    (pos = dynmap_value(unsigned *, st->type_ref_to_off, i));
	    i++)
	{
		free(pos);
	}

	dynmap_free(st->type_ref_to_off);
	st->type_ref_to_off = NULL;

	dwarf_sec_free(&st->abbrev);
	dwarf_sec_free(&st->info);
}

static int type_ref_cmp_bool(void *a, void *b)
{
	return type_ref_cmp(a, b, 0) != TYPE_EQUAL;
}

void out_dbginfo(symtable_global *globs, const char *fname)
{
	struct dwarf_state st = {
		DWARF_SEC_INIT(),
		DWARF_SEC_INIT(),
		NULL
	};

	/* initialise type offset maps */
	st.type_ref_to_off = dynmap_new(&type_ref_cmp_bool);

	dwarf_info_header(&st.info, cc_out[SECTION_DBG_INFO]);

	/* output abbrev Compile Unit header */
	dwarf_cu(&st, fname);

	/* output subprograms */
	{
		decl **diter;
		for(diter = globs->stab.decls; diter && *diter; diter++){
			decl *d = *diter;

			dwarf_help(&st, "decl %s", decl_to_str(d));
			if(DECL_IS_FUNC(d)){
				; /* TODO: dwarf_subprogram_func(&st, d); */
			}else{
				/* TODO: dump type unless seen */
				dwarf_global_variable(&st, d);
			}
		}
	}

	dwarf_flush(&st.abbrev, cc_out[SECTION_DBG_ABBREV]);
	dwarf_flush(&st.info, cc_out[SECTION_DBG_INFO]);

	dwarf_info_footer(&st.info, cc_out[SECTION_DBG_INFO]);

	dwarf_st_free(&st);
}
