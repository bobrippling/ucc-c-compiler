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

static void asm_declare_pad(FILE *f, unsigned pad, const char *why)
{
	if(pad)
		fprintf(f, ".space %u # %s\n", pad, why);
}

static void asm_declare_init(FILE *f, decl_init *init, type_ref *tfor)
{
	type_ref *r;

	if(init == DYNARRAY_NULL)
		init = NULL;

	if((r = type_ref_is_type(tfor, type_struct))){
		/* array of stmts for each member
		 * assumes the ->bits.inits order is member order
		 */
		sue_member **mem;
		decl_init **i;
		int end_of_last = 0;

		if(init){
			UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");
			i = init->bits.inits;
		}else{
			i = NULL;
		}

		/* iterate using members, not inits */
		for(mem = r->bits.type->sue->members;
				mem && *mem;
				mem++)
		{
			decl *d_mem = (*mem)->struct_member;

			asm_declare_pad(f, d_mem->struct_offset - end_of_last, "struct padding");

			asm_declare_init(f, i ? *i : NULL, d_mem->ref);

			if(i && !*++i)
				i = NULL; /* reached end */

			end_of_last = d_mem->struct_offset + type_ref_size(d_mem->ref, NULL);
		}

	}else if((r = type_ref_is(tfor, type_ref_array))){
		if(init){
			size_t i;
			decl_init **p, *last_nonflag = NULL;
			type_ref *next = type_ref_next(tfor);

			UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");
			UCC_ASSERT(type_ref_is_complete(tfor), "incomplete array init");

			for(i = type_ref_array_len(tfor), p = init->bits.inits;
					i > 0;
					i--)
			{
				decl_init *this;
				if(*p){
					this = *p++;

					if(this == DYNARRAY_FLAG){
						UCC_ASSERT(last_nonflag, "no previous init for array range");
						this = last_nonflag;
					}else{
						last_nonflag = this;
					}
				}else{
					last_nonflag = this = NULL;
				}

				asm_declare_init(f, this, next);
			}
		}else{
			/* we should have a size */
			asm_declare_pad(f, type_ref_size(r, NULL), "empty array");
		}

	}else if((r = type_ref_is_type(tfor, type_union))){
		/* union inits are decl_init_brace with spaces up to the first union init,
		 * then NULL/end of the init-array */
		struct_union_enum_st *sue = type_ref_is_s_or_u(r);
		unsigned i, sub = 0;

		UCC_ASSERT(init->type == decl_init_brace, "brace init expected");

		/* skip the empties until we get to one */
		for(i = 0; init->bits.inits[i] == DYNARRAY_NULL; i++);

		if(init->bits.inits[i]){
			/* null union init */
			type_ref *mem_r = sue->members[i]->struct_member->ref;

			/* union init, member at index `i' */
			asm_declare_init(f, init->bits.inits[i], mem_r);

			sub = type_ref_size(mem_r, NULL);
		}

		asm_declare_pad(f,
				type_ref_size(r, NULL) - sub,
				"union extra");

	}else{
		if(!init){
			asm_declare_pad(f, type_ref_size(tfor, NULL), "null scalar init");

		}else{
			/* scalar */
			expr *exp = init->bits.expr;

			UCC_ASSERT(init->type == decl_init_scalar, "scalar init expected");

			if(exp == DYNARRAY_NULL)
				exp = NULL;

			/* exp->tree_type should match tfor */
			{
				char buf[TYPE_REF_STATIC_BUFSIZ];

				UCC_ASSERT(type_ref_equal(exp->tree_type, tfor,
							DECL_CMP_ALLOW_VOID_PTR | DECL_CMP_ALLOW_SIGNED_UNSIGNED),
						"mismatching init types: %s and %s",
						type_ref_to_str_r(buf, exp->tree_type),
						type_ref_to_str(tfor));
			}

			/* use tfor, since "abc" has type (char[]){(int)'a', (int)'b', ...} */
			fprintf(f, ".%s ", asm_type_directive(tfor));
			static_addr(exp);
			fputc('\n', f);
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

void asm_predeclare_extern(decl *d)
{
	(void)d;
	/*
	asm_comment("extern %s", d->spel);
	asm_out_section(SECTION_BSS, "extern %s", d->spel);
	*/
}

void asm_predeclare_global(decl *d)
{
	/* FIXME: section cleanup - along with __attribute__((section("..."))) */
	asm_out_section(SECTION_TEXT, ".globl %s\n", decl_asm_spel(d));
}

void asm_declare_decl_init(FILE *f, decl *d)
{
	if((d->store & STORE_MASK_STORE) == store_extern){
		asm_predeclare_extern(d);

	}else if(d->init && !decl_init_is_zero(d->init)){
		fprintf(f, ".align %d\n", type_ref_align(d->ref, NULL));
		fprintf(f, "%s:\n", decl_asm_spel(d));
		asm_declare_init(f, d->init, d->ref);
		fputc('\n', f);

	}else{
		/* always resB, since we use decl_size() */
		asm_reserve_bytes(decl_asm_spel(d), decl_size(d, &d->where));

	}
}

void asm_out_section(enum section_type t, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(cc_out[t], fmt, l);
	va_end(l);
}
