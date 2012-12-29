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
#include "../../util/dynarray.h"
#include "../sue.h"
#include "../const.h"
#include "../gen_asm.h"
#include "../data_store.h"
#include "../decl_init.h"

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

int asm_table_lookup(type_ref *r)
{
	int sz;
	int i;

	if(!r)
		return ASM_INDEX_LONG;

	/* special case for funcs and arrays */
	if(type_ref_is(r, type_ref_array) || type_ref_is(r, type_ref_func))
		sz = platform_word_size();
	else
		sz = type_ref_size(r, NULL);

	for(i = 0; i <= ASM_TABLE_MAX; i++)
		if(asm_type_table[i].sz == sz)
			return i;

	ICE("no asm type index for byte size %d", sz);
	return -1;
}

char asm_type_ch(type_ref *r)
{
	return asm_type_table[asm_table_lookup(r)].ch;
}

const char *asm_type_directive(type_ref *r)
{
	return asm_type_table[asm_table_lookup(r)].directive;
}

void asm_reg_name(type_ref *r, const char **regpre, const char **regpost)
{
	const int i = asm_table_lookup(r);
	*regpre  = asm_type_table[i].regpre;
	*regpost = asm_type_table[i].regpost;
}

int asm_type_size(type_ref *r)
{
	return asm_type_table[asm_table_lookup(r)].sz;
}

void asm_declare_partial(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	vfprintf(cc_out[SECTION_DATA], fmt, l);
	va_end(l);
}

#if 0
static void asm_declare_array(const char *lbl, array_decl *ad)
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

	asm_out_section(SECTION_DATA, "%s:\n", lbl);

	for(i = 0; i < ad->len; i++){
		if(ad->type == array_str){
			asm_out_section(SECTION_DATA, ".%s %d\n",
					asm_type_table[tbl_idx].directive,
					ad->data.str[i]);
		}else{
			static_store(ad->data.exprs[i]);
		}
	}
}
#endif

static void asm_declare_sub(FILE *f, decl_init *init)
{
	switch(init->type){
		case decl_init_brace:
		{
			decl_init **const inits = init->bits.inits;
			const int len = dynarray_count((void **)inits);
			int i;

			for(i = 0; i < len; i++){
				static int warned;

				asm_declare_sub(f, inits[i]);
				fputc('\n', f);

				/* TODO: struct padding for next member */
				if(!warned){
					ICW("global struct padding TODO"), warned = 1;
					fputs("// TODO: struct padding for next\n", f);
				}
			}
			break;
		}

		case decl_init_scalar:
		{
			expr *const exp = init->bits.expr;

			fprintf(f, ".%s ", asm_type_directive(exp->tree_type));

			if(exp->data_store) /* FIXME: should be handled in expr_addr::static_store */
				data_store_out(exp->data_store, 0);
			else
				static_addr(exp); /*if(!const_expr_is_zero(exp))...*/

			break;
		}
	}
}

static void asm_reserve_bytes(const char *lbl, int nbytes)
{
	/*
	 * TODO: .comm buf,512,5
	 * or    .zerofill SECTION_NAME,buf,512,5
	 */
	asm_out_section(SECTION_BSS, "%s:\n", lbl);

	while(nbytes > 0){
		int i;

		for(i = ASM_TABLE_MAX; i >= 0; i--){
			const int sz = asm_type_table[i].sz;

			if(nbytes >= sz){
				asm_out_section(SECTION_BSS, ".%s 0\n", asm_type_table[i].directive);
				nbytes -= sz;
				break;
			}
		}
	}
}

void asm_declare(FILE *f, decl *d)
{
	if(d->init /* FIXME: should also check for non-zero... */){
		fprintf(f, "%s:\n", d->spel);
		asm_declare_sub(f, d->init);
		fputc('\n', f);

	}else if(d->store == store_extern){
		gen_asm_extern(d);

	}else{
		/* always resB, since we use decl_size() */
		asm_reserve_bytes(d->spel, decl_size(d, &d->where));

	}
}

void asm_out_section(enum section_type t, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(cc_out[t], fmt, l);
	va_end(l);
}
