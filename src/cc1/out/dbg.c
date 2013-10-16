#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../../util/where.h"

#include "../str.h"

#include "../data_structs.h"
#include "../expr.h"
#include "../tree.h"

#include "../cc1.h" /* cc_out[] */

#include "dbg.h"

struct dwarf_state
{
	struct dwarf_sec
	{
		FILE *f;
		int idx;
		int indent;
	} abbrev, info;
};

enum dwarf_key
{
	DW_TAG_base_type = 0x24,
	DW_AT_byte_size = 0xb,
	DW_AT_encoding = 0x3e,
	DW_AT_name = 0x3,
	DW_AT_language = 0x13,
	DW_AT_low_pc = 0x11,
	DW_AT_high_pc = 0x12,
	DW_AT_producer = 0x25,
};
enum dwarf_valty
{
	DW_FORM_addr = 0x1,
	DW_FORM_data1 = 0xb,
	DW_FORM_data2 = 0x5,
	DW_FORM_string = 0x8,
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

static void indent(FILE *f, int idt)
{
	while(idt --> 0)
		fputc('\t', f);
}

static void dwarf_sec_byte(struct dwarf_sec *sec, int byte, ...) /* -1 terminator */
{
	const char *join = " ";
	va_list l;

	va_start(l, byte);

	indent(sec->f, sec->indent);

	fprintf(sec->f, ".byte");

	do{
		fprintf(sec->f, "%s%d", join, byte);
		join = ", ";

		byte = va_arg(l, int);
	}while(byte != -1);

	va_end(l);

	fputc('\n', sec->f);
}

static void dwarf_attr(
		struct dwarf_state *st,
		enum dwarf_key key, enum dwarf_valty val,
		...)
{
	va_list l;

	/* abbrev part */
	dwarf_sec_byte(&st->abbrev, key, val, -1);

	indent(st->info.f, st->info.indent);

	/* info part */
	va_start(l, val);
	switch(val){
		case DW_FORM_addr:
			fprintf(st->info.f, ".quad 0x%lx", (long)va_arg(l, void *));
			break;
		case DW_FORM_data1:
			fprintf(st->info.f, ".byte %d", va_arg(l, int));
			break;
		case DW_FORM_data2:
			fprintf(st->info.f, ".word %d", va_arg(l, int));
			break;
		case DW_FORM_string:
		{
			char *esc = va_arg(l, char *);
			esc = str_add_escape(esc, strlen(esc));
			fprintf(st->info.f, ".asciz \"%s\"", esc);
			free(esc);
			break;
		}
	}
	va_end(l);

	fputc('\n', st->info.f);
}

static void dwarf_sec_start(struct dwarf_sec *sec)
{
	dwarf_sec_byte(sec, sec->idx++, -1);
	sec->indent++;
}

static void dwarf_sec_end(struct dwarf_sec *sec)
{
	sec->indent--;
	dwarf_sec_byte(sec, 0, -1);
}

static void dwarf_start(struct dwarf_state *st)
{
	dwarf_sec_start(&st->abbrev);
	dwarf_sec_start(&st->info);
}

static void dwarf_end(struct dwarf_state *st)
{
	dwarf_sec_end(&st->abbrev);
	st->info.indent--; /* we don't terminate info entries */
}

static void dwarf_abbrev_start(struct dwarf_state *st, int b1, int b2)
{
	dwarf_sec_byte(&st->abbrev, b1, b2, -1);
	st->abbrev.indent++;
}

static void dwarf_basetype(struct dwarf_state *st, enum type_primitive prim, int enc)
{
	dwarf_start(st);
		dwarf_abbrev_start(st, DW_TAG_base_type, DW_CHILDREN_no);
			dwarf_attr(st, DW_AT_name,      DW_FORM_string, type_primitive_to_str(prim));
			dwarf_attr(st, DW_AT_byte_size, DW_FORM_data1,  type_primitive_size(prim));
			dwarf_attr(st, DW_AT_encoding,  DW_FORM_data1,  enc);
		dwarf_sec_end(&st->abbrev);
	dwarf_end(st);
}

static void dwarf_cu(struct dwarf_state *st, const char *fname)
{
	dwarf_start(st);
		dwarf_abbrev_start(st, DW_TAG_compile_unit, DW_CHILDREN_yes);
			dwarf_attr(st, DW_AT_producer, DW_FORM_string, "ucc development version");
			dwarf_attr(st, DW_AT_language, DW_FORM_data2, DW_LANG_C99);
			dwarf_attr(st, DW_AT_name, DW_FORM_string, fname);
			dwarf_attr(st, DW_AT_low_pc, DW_FORM_addr, 0x12345); /* TODO */
			dwarf_attr(st, DW_AT_high_pc, DW_FORM_addr, 0x54321);
		dwarf_sec_end(&st->abbrev);
	dwarf_end(st);
}

static void dwarf_info_header(struct dwarf_sec *sec)
{
	/* hacky? */
	fprintf(sec->f,
			"\t.long .Ldbg_info_end - .Ldbg_info_start\n"
			".Ldbg_info_start:\n"
			"\t.short 2 # DWARF 2\n"
			"\t.long 0  # abbrev offset\n"
			"\t.byte 8  # sizeof(void *)\n");
}

static void dwarf_info_footer(struct dwarf_sec *sec)
{
	fprintf(sec->f, ".Ldbg_info_end:\n");
}

void out_dbginfo(symtable_global *globs, const char *fname)
{
	struct dwarf_state st = {
		{ cc_out[SECTION_DBG_ABBREV], 1, 1 },
		{ cc_out[SECTION_DBG_INFO],   1, 1 }
	};

	dwarf_info_header(&st.info);

	/* output abbrev Compile Unit header */
	dwarf_cu(&st, fname);

	/* output btypes - FIXME: need a nice way to iterate over types */
	/* TODO: unsigned types */
	dwarf_basetype(&st, type__Bool, DW_ATE_signed);
	dwarf_basetype(&st, type_short, DW_ATE_signed);
	dwarf_basetype(&st, type_int, DW_ATE_signed);
	dwarf_basetype(&st, type_long, DW_ATE_signed);
	dwarf_basetype(&st, type_llong, DW_ATE_signed);

	dwarf_basetype(&st, type_char, DW_ATE_signed_char);

	dwarf_basetype(&st, type_float, DW_ATE_float);
	dwarf_basetype(&st, type_double, DW_ATE_float);
	dwarf_basetype(&st, type_ldouble, DW_ATE_float);

#if 0
	/* output subprograms */
	for(diter = globs->stab.decls; diter && *diter; diter++){
		decl *d = *diter;

		if(DECL_IS_FUNC(d))
			dwarf_subprogram_func(&st, d);
		else
			; /* TODO: global variables */
	}
#else
	(void)globs;
#endif

	dwarf_info_footer(&st.info);
}
