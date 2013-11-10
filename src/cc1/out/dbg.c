#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../../util/where.h"
#include "../../util/platform.h"
#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "../str.h"

#include "../data_structs.h"
#include "../expr.h"
#include "../tree.h"
#include "../const.h"
#include "../funcargs.h"
#include "../sue.h"

#include "../cc1.h" /* cc_out[] */

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
				DWARF_STR  = 9
			} val_type;
			union
			{
				unsigned long long value;
				char *str; /* FREE */
			} bits;

			int indent_adj;
		} *values; /* FREE */
		size_t nvalues;
		size_t length;

		int last_idx;
	} abbrev, info;
#define DWARF_SEC_INIT() { NULL, 0, 0, 1 }
};

struct dwarf_block /* DW_FORM_block1 */
{
	unsigned len;
	unsigned *vals;
};

enum dwarf_block_ops
{
	DW_OP_plus_uconst = 0x23
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

	DW_AT_byte_size = 0xb,
	DW_AT_encoding = 0x3e,
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
	sec->values = urealloc1(sec->values, ++sec->nvalues * sizeof *sec->values);
	sec->values[sec->nvalues-1].indent_adj = 0;
	return &sec->values[sec->nvalues - 1];
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

static void dwarf_add_str(struct dwarf_sec *sec, char *str)
{
	struct dwarf_val *val = dwarf_value_new(sec);

	val->val_type = DWARF_STR;
	val->bits.str = str;

	sec->length += strlen(str) + 1;
}

static void dwarf_sec_indent(struct dwarf_sec *sec, int chg)
{
	sec->values[sec->nvalues-1].indent_adj = chg;
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
			unsigned i;

			dwarf_add_value(&st->info, /*byte:*/1, (signed char)blk->len);

			for(i = 0; i < blk->len; i++)
				dwarf_add_value(&st->info, /*byte:*/1, blk->vals[i]);
			break;
		}
		case DW_FORM_ref4:
			dwarf_add_value(&st->info, /*long:*/4, va_arg(l, unsigned));
			break;
		case DW_FORM_addr:
			dwarf_add_value(&st->info, /*quad:*/8, (long)va_arg(l, void *));
			break;
		case DW_FORM_data1:
		case DW_FORM_flag:
			dwarf_add_value(&st->info, /*byte:*/1, (signed char)va_arg(l, int));
			break;
		case DW_FORM_data2:
			dwarf_add_value(&st->info, /*word:*/2, (short)va_arg(l, int));
			break;
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

static void dwarf_basetype(struct dwarf_state *st, enum type_primitive prim, int enc)
{
	dwarf_start(st); {
		dwarf_abbrev_start(st, DW_TAG_base_type, DW_CHILDREN_no); {
			dwarf_attr(st, DW_AT_name,      DW_FORM_string, type_primitive_to_str(prim));
			dwarf_attr(st, DW_AT_byte_size, DW_FORM_data1,  type_primitive_size(prim));
			dwarf_attr(st, DW_AT_encoding,  DW_FORM_data1,  enc);
		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static void dwarf_sue_header(
		struct dwarf_state *st, struct_union_enum_st *sue,
		int dwarf_tag, int children)
{
	dwarf_start(st); {
		dwarf_abbrev_start(st, dwarf_tag, children ? DW_CHILDREN_yes : DW_CHILDREN_no); {
			/*dwarf_attr(st, DW_AT_sibling, ... next?);*/
			if(!sue->anon)
				dwarf_attr(st, DW_AT_name, DW_FORM_string, sue->spel);
			if(sue_complete(sue))
				dwarf_attr(st, DW_AT_byte_size, DW_FORM_data1, sue_size(sue, NULL));
		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static unsigned dwarf_type(struct dwarf_state *st, type_ref *ty)
{
	unsigned this_start;

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

						dwarf_sue_header(st, sue, DW_TAG_enumeration_type, /*children:*/0);

						ICW_1("need to make enumerators siblings of the enumeration");

						/* enumerators */
						for(i = sue->members; i && *i; i++){
							dwarf_start(st); {
								dwarf_abbrev_start(st, DW_TAG_enumerator, DW_CHILDREN_no); {
									enum_member *emem = (*i)->enum_member;

									dwarf_attr(st,
											DW_AT_name, DW_FORM_string,
											emem->spel);

									dwarf_attr(st,
											DW_AT_const_value, DW_FORM_data1,
											(int)const_fold_val(emem->val));

								} dwarf_sec_end(&st->abbrev);
							} dwarf_end(st);
						}
						break;
					}

					case type_union:
					case type_struct:
					{
						const size_t nmem = dynarray_count(sue->members);
						sue_member **si;
						unsigned i;
						unsigned *mem_offsets = nmem ? umalloc(nmem * sizeof *mem_offsets) : NULL;

						ICW_1("need to make members siblings of the struct/union");

						/* member types */
						for(i = 0; i < nmem; i++)
							mem_offsets[i] = dwarf_type(st, sue->members[i]->struct_member->ref);

						/* must update since we might've output extra type information */
						this_start = st->info.length;

						dwarf_sue_header(
								st, sue,
								sue->primitive == type_struct
									? DW_TAG_structure_type
									: DW_TAG_union_type,
								/*children:*/0);

						/* members */
						for(i = 0, si = sue->members; i < nmem; i++, si++){
							dwarf_start(st); {
								dwarf_abbrev_start(st, DW_TAG_member, DW_CHILDREN_no); {
									struct dwarf_block offset;
									unsigned offset_data[2];

									decl *dmem = (*si)->struct_member;

									dwarf_attr(st,
											DW_AT_name, DW_FORM_string,
											dmem->spel);

									dwarf_attr(st,
											DW_AT_type, DW_FORM_ref4,
											mem_offsets[i]);

									/* TODO: bitfields */
									offset_data[0] = DW_OP_plus_uconst;
									offset_data[1] = dmem->struct_offset;

									offset.len = 2;
									offset.vals = offset_data;

									dwarf_attr(st,
											DW_AT_data_member_location, DW_FORM_block1,
											&offset);

									dwarf_attr(st,
											DW_AT_accessibility, DW_FORM_flag,
											DW_ACCESS_public);

								} dwarf_sec_end(&st->abbrev);
							} dwarf_end(st);
						}

						free(mem_offsets);
						break;
					}
				}

			}else{
				/* TODO: unsigned type */
				dwarf_basetype(st, ty->bits.type->primitive,
						type_ref_is_floating(ty) ? DW_ATE_float : DW_ATE_signed);
			}
			break;
		}

		case type_ref_tdef:
			if(ty->bits.tdef.decl){
				decl *d = ty->bits.tdef.decl;
				const unsigned sub_pos = dwarf_type(st, d->ref);

				this_start = st->info.length;

				dwarf_start(st); {
					dwarf_abbrev_start(st, DW_TAG_typedef, DW_CHILDREN_yes); {
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
				dwarf_abbrev_start(st, DW_TAG_pointer_type, DW_CHILDREN_yes); {
					dwarf_attr(st, DW_AT_byte_size, DW_FORM_data1, platform_word_size());
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
				dwarf_abbrev_start(st, DW_TAG_subroutine_type, DW_CHILDREN_yes); {
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
			intval_t sz;
			const unsigned sub_pos = dwarf_type(st, ty->ref);

			this_start = st->info.length;

			if(have_sz)
				sz = const_fold_val(ty->bits.array.size);

			dwarf_start(st); {
				dwarf_abbrev_start(st, DW_TAG_array_type, DW_CHILDREN_yes); {
					dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
					/*dwarf_attr(st, DW_AT_sibling, DW_FORM_ref4, "???");*/
				} dwarf_sec_end(&st->abbrev);

				if(have_sz){
					dwarf_abbrev_start(st, DW_TAG_subrange_type, DW_CHILDREN_yes); {
						dwarf_attr(st, DW_AT_lower_bound, DW_FORM_data1, 0);
						dwarf_attr(st, DW_AT_upper_bound, DW_FORM_data1, sz);
					} dwarf_sec_end(&st->abbrev);
				}
			} dwarf_end(st);
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
					dwarf_abbrev_start(st, DW_TAG_const_type, DW_CHILDREN_yes); {
						dwarf_attr(st, DW_AT_type, DW_FORM_ref4, sub_pos);
					} dwarf_sec_end(&st->abbrev);
				} dwarf_end(st);
			}
			break;
		}
	}

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
	unsigned typos;

	if(!d->spel)
		return;

	typos = dwarf_type(st, d->ref);

	dwarf_start(st); {
		dwarf_abbrev_start(st, DW_TAG_variable, DW_CHILDREN_no); {
			dwarf_attr(st, DW_AT_name, DW_FORM_string, d->spel);
			dwarf_attr(st, DW_AT_type, DW_FORM_ref4, typos);
			/*dwarf_attr(st, DW_AT_location, DW_FORM_block1, d->spel_asm);*/
		} dwarf_sec_end(&st->abbrev);
	} dwarf_end(st);
}

static void dwarf_flush(struct dwarf_sec *sec, FILE *f)
{
	unsigned indent = 0;
	size_t i;

	for(i = 0; i < sec->nvalues; i++){
		struct dwarf_val *val = &sec->values[i];
		const char *ty = NULL;
		unsigned indent_adj;

		switch(val->val_type){
			case DWARF_BYTE: ty = "byte"; break;
			case DWARF_WORD: ty = "word"; break;
			case DWARF_LONG: ty = "long"; break;
			case DWARF_QUAD: ty = "quad"; break;
			case DWARF_STR: break;
		}

		/* increments take affect before, decrements, after */
		if(val->indent_adj > 0)
			indent += val->indent_adj;
		for(indent_adj = 0; indent_adj < indent; indent_adj++)
			fputc('\t', f);
		if(val->indent_adj < 0)
			indent += val->indent_adj;

		if(ty){
			fprintf(f, ".%s %lld\n", ty, val->bits.value);
		}else{
			fprintf(f, ".asciz \"%s\"\n", val->bits.str);
		}
	}
}

void out_dbginfo(symtable_global *globs, const char *fname)
{
	struct dwarf_state st = {
		DWARF_SEC_INIT(),
		DWARF_SEC_INIT()
	};

	dwarf_info_header(&st.info, cc_out[SECTION_DBG_INFO]);

	/* output abbrev Compile Unit header */
	dwarf_cu(&st, fname);

	/* output subprograms */
	{
		decl **diter;
		for(diter = globs->stab.decls; diter && *diter; diter++){
			decl *d = *diter;

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
}
