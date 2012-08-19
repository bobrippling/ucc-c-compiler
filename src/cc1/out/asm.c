#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../data_structs.h"
#include "../cc1.h"
#include "../sym.h"
#include "asm.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"
#include "../sue.h"
#include "../const.h"

static const struct
{
	int sz;
	char ch;
	const char *directive;
	const char *regpre, *regpost;
} asm_type_table[] = {
	{ 1,  'b', "byte", "",  "l"  },
	{ 2,  'w', "word", "",  "x"  },
	{ 4,  'l', "long", "e", "x" },
	{ 8,  'q', "quad", "r", "x" },

	/* llong */
	{ 16,  '\0', "???", "r", "x" },

	/* ldouble */
	{ 10,  '\0', "???", "r", "x" },
};
#define ASM_TABLE_MAX 3

enum
{
	ASM_INDEX_CHAR    = 0,
	ASM_INDEX_SHORT   = 1,
	ASM_INDEX_INT     = 2,
	ASM_INDEX_LONG    = 3,
	ASM_INDEX_LLONG   = 4,
	ASM_INDEX_LDOUBLE = 5,

	ASM_INDEX_PTR = ASM_INDEX_LONG,
};

#if 0
const char *asm_intval_str(intval *iv)
{
	static char buf[64];
	char fmt[8];
	char *p = fmt;

	*p++ = '$'; /* $53 */
	*p++ = '%';
	if(iv->suffix & VAL_LONG)
		*p++ = 'l';

	strcpy(p, iv->suffix & VAL_UNSIGNED ? "u" : "d");

	snprintf(buf, sizeof buf, fmt, iv->val);
	return buf;
}
#endif

void asm_declare_single_part(expr *e)
{
	if(!e->f_gen_1)
		ICE("no gen_1() for expr %s (%s)", e->f_str(), decl_to_str(e->tree_type));

	e->f_gen_1(e, cc_out[SECTION_DATA]);
}

int asm_table_lookup(decl *d)
{
	if(!d || decl_ptr_or_block(d)){
		return ASM_INDEX_PTR;
	}else{
		if(d->type->typeof)
			ICE("typedefs should've been folded by now");

		switch(d->type->primitive){
			case type_void:
				ICE("type primitive is void (\"%s\")", decl_to_str(d));

			case type__Bool:
			case type_char:
				return ASM_INDEX_CHAR;

			case type_short:
				return ASM_INDEX_SHORT;

			case type_int:
			case type_float:
				return ASM_INDEX_INT;

			case type_ldouble:
				ICE("long double in asm");
				return ASM_INDEX_LDOUBLE;
			case type_llong:
				ICE("long long in asm");
				return ASM_INDEX_LLONG;

			case type_double:
			case type_long:
				return ASM_INDEX_LONG;

			case type_enum:
			case type_struct:
			case type_union:
				ICE("%s of %s (%s)",
						__func__,
						sue_str(d->type->sue),
						decl_to_str(d));
				/*DIE_AT(&d->where, "invalid use of struct (%s:%d)", __FILE__, __LINE__);*/

			case type_unknown:
				ICE("type primitive not set");
		}
	}
	ICE("%s switch error", __func__);
	return 0;
}

char asm_type_ch(decl *d)
{
	return asm_type_table[asm_table_lookup(d)].ch;
}

const char *asm_type_directive(decl *d)
{
	return asm_type_table[asm_table_lookup(d)].directive;
}

void asm_reg_name(decl *d, const char **regpre, const char **regpost)
{
	const int i = asm_table_lookup(d);
	*regpre  = asm_type_table[i].regpre;
	*regpost = asm_type_table[i].regpost;
}

int asm_type_size(decl *d)
{
	struct_union_enum_st *sue = d->type->sue;

	if(sue && !decl_ptr_depth(d))
		return sue_size(sue);

	return asm_type_table[asm_table_lookup(d)].sz;
}

void asm_declare_out(FILE *f, decl *d, const char *fmt, ...)
{
	va_list l;

	fprintf(f, ".%s ", asm_type_directive(d));

	va_start(l, fmt);
	vfprintf(f, fmt, l);
	va_end(l);

	fputc('\n', f);
}

void asm_declare_single(decl *d)
{
	if(!decl_ptr_depth(d) && d->type->sue && d->type->sue->primitive != type_enum){
		/* struct init */
		int i;
		const char *type_directive;

		ICW("attempting to declare+init a struct, untested for nesting and arrays");

		UCC_ASSERT(d->init->array_store, "no array store for struct init (TODO?)");
		UCC_ASSERT(d->init->array_store->type == array_exprs, "array store of strings for struct");

		asm_out_section(SECTION_DATA, "%s:", d->spel);

		/* XXX: assumes all struct members are word-size */
		type_directive = asm_type_table[asm_table_lookup(NULL)].directive;

		for(i = 0; i < d->init->array_store->len; i++){
			intval iv;

			const_fold_need_val(d->init->array_store->data.exprs[i], &iv);

			asm_out_section(SECTION_DATA, ".%s %ld", type_directive, iv.val);
		}
	}else{
		asm_out_section(SECTION_DATA, "%s:", d->spel);
		asm_declare_single_part(d->init);
	}
}

void asm_declare_array(const char *lbl, array_decl *ad)
{
	int tbl_idx;
	int i;

	switch(ad->type){
		case array_str:
			tbl_idx = ASM_INDEX_CHAR;
			break;
		case array_exprs:
			tbl_idx = asm_table_lookup(NULL);
			break;
	}

	asm_out_section(SECTION_DATA, "%s:", lbl);

	for(i = 0; i < ad->len; i++){
		if(ad->type == array_str){
			asm_out_section(SECTION_DATA, ".%s %d",
					asm_type_table[tbl_idx].directive,
					ad->data.str[i]);
		}else{
			asm_declare_single_part(ad->data.exprs[i]);
		}
	}
}

void asm_reserve_bytes(const char *lbl, int nbytes)
{
	asm_out_section(SECTION_BSS, "%s:", lbl);

	while(nbytes > 0){
		int i;

		for(i = ASM_TABLE_MAX; i >= 0; i--){
			const int sz = asm_type_table[i].sz;

			if(nbytes >= sz){
				asm_out_section(SECTION_BSS, ".%s 0", asm_type_table[i].directive);
				nbytes -= sz;
				break;
			}
		}
	}
}

void asm_out_section(enum section_type t, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(cc_out[t], fmt, l);
	va_end(l);
	fputc('\n', cc_out[t]);
}
