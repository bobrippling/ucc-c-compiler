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

#define ASSERT_SCALAR(di)                  \
	UCC_ASSERT(di->type == decl_init_scalar, \
			"scalar expected for bitfield init")

struct bitfield_val
{
	intval_t val;
	unsigned offset;
	unsigned width;
};

int asm_table_lookup(type_ref *r)
{
	int sz;
	int i;

	if(!r)
		sz = type_primitive_size(type_long); /* or ptr */
	else if(type_ref_is(r, type_ref_array) || type_ref_is(r, type_ref_func))
		/* special case for funcs and arrays */
		sz = platform_word_size();
	else
		sz = type_ref_size(r, NULL);

	for(i = 0; i < ASM_TABLE_LEN; i++)
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

int asm_type_size(type_ref *r)
{
	return asm_type_table[asm_table_lookup(r)].sz;
}

static void asm_declare_pad(FILE *f, unsigned pad, const char *why)
{
	if(pad)
		fprintf(f, ".space %u # %s\n", pad, why);
}

static void asm_declare_init_type(FILE *f, type_ref *ty)
{
	fprintf(f, ".%s ", asm_type_directive(ty));
}

static void asm_declare_init_bitfields(
		FILE *f,
		struct bitfield_val *vals, unsigned n,
		type_ref *ty)
{
	intval_t v = 0;
	unsigned i;

	for(i = 0; i < n; i++){
		intval_t this = intval_truncate_bits(
				vals[i].val, vals[i].width);

		v |= this << vals[i].offset;
	}

	asm_declare_init_type(f, ty);
	fprintf(f, "%" INTVAL_FMT_D "\n", v);
}

static void bitfields_out(
		FILE *f,
		struct bitfield_val *bfs, unsigned *pn,
		type_ref *ty)
{
	asm_declare_init_bitfields(f, bfs, *pn, ty);
	*pn = 0;
}

static struct bitfield_val *bitfields_add(
		struct bitfield_val *bfs, unsigned *pn,
		decl *mem, decl_init *di)
{
	const unsigned i = *pn;

	bfs = urealloc1(bfs, (*pn += 1) * sizeof *bfs);

	if(di)
		ASSERT_SCALAR(di);

	bfs[i].val = di ? const_fold_val(di->bits.expr) : 0;
	bfs[i].offset = mem->struct_offset_bitfield;
	bfs[i].width = const_fold_val(mem->field_width);

	return bfs;
}

static void asm_declare_init(FILE *f, decl_init *init, type_ref *tfor)
{
	type_ref *r;

	if(init == DYNARRAY_NULL)
		init = NULL;

	if(!init){
		asm_declare_pad(f, type_ref_size(tfor, NULL),
				"null init"/*, type_ref_to_str(tfor)*/);

	}else if((r = type_ref_is_type(tfor, type_struct))){
		/* array of stmts for each member
		 * assumes the ->bits.inits order is member order
		 */
		sue_member **mem;
		decl_init **i;
		int end_of_last = 0;
		struct bitfield_val *bitfields = NULL;
		unsigned nbitfields = 0;
		decl *first_bf = NULL;

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");
		i = init->bits.ar.inits;

		/* iterate using members, not inits */
		for(mem = r->bits.type->sue->members;
				mem && *mem;
				mem++)
		{
			decl *d_mem = (*mem)->struct_member;

			/* only pad if we're not on a bitfield or we're on the first bitfield */
			if(!d_mem->field_width || !first_bf)
				asm_declare_pad(f, d_mem->struct_offset - end_of_last, "struct padding");

			if(d_mem->field_width){
				if(!first_bf)
					first_bf = d_mem;

				bitfields = bitfields_add(
						bitfields, &nbitfields,
						d_mem, i ? *i : NULL);
			}else{
				if(nbitfields){
					bitfields_out(f, bitfields, &nbitfields, first_bf->ref);
					first_bf = NULL;
				}

				asm_declare_init(f, i ? *i : NULL, d_mem->ref);
			}

			if(i && !*++i)
				i = NULL; /* reached end */

			end_of_last = d_mem->struct_offset + type_ref_size(d_mem->ref, NULL);
		}

		if(nbitfields)
			bitfields_out(f, bitfields, &nbitfields, first_bf->ref);
		free(bitfields);

	}else if((r = type_ref_is(tfor, type_ref_array))){
		size_t i;
		decl_init **p;
		type_ref *next = type_ref_next(tfor);

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");
		UCC_ASSERT(type_ref_is_complete(tfor), "incomplete array init");

		for(i = type_ref_array_len(tfor), p = init->bits.ar.inits;
				i > 0;
				i--)
		{
			decl_init *this = NULL;
			if(*p){
				this = *p++;

				if(this != DYNARRAY_NULL && this->type == decl_init_copy){
					/*fprintf(f, "# copy from %lu\n", DECL_INIT_COPY_IDX(this, init));*/
					struct init_cpy *icpy = *this->bits.range_copy;
					/* resolve the copy */
					this = icpy->range_init;
				}
			}

			asm_declare_init(f, this, next);
		}

	}else if((r = type_ref_is_type(tfor, type_union))){
		/* union inits are decl_init_brace with spaces up to the first union init,
		 * then NULL/end of the init-array */
		struct_union_enum_st *sue = type_ref_is_s_or_u(r);
		unsigned i, sub = 0;
		decl_init *u_init;

		UCC_ASSERT(init->type == decl_init_brace, "brace init expected");

		/* skip the empties until we get to one */
		for(i = 0; init->bits.ar.inits[i] == DYNARRAY_NULL; i++);

		if((u_init = init->bits.ar.inits[i])){
			decl *mem = sue->members[i]->struct_member;
			type_ref *mem_r = mem->ref;

			/* union init, member at index `i' */
			if(mem->field_width){
				/* we know it's integral */
				struct bitfield_val bfv;

				ASSERT_SCALAR(u_init);

				bfv.val = const_fold_val(u_init->bits.expr);
				bfv.offset = 0;
				bfv.width = const_fold_val(mem->field_width);

				asm_declare_init_bitfields(f, &bfv, 1, mem_r);
			}else{
				asm_declare_init(f, u_init, mem_r);
			}

			sub = type_ref_size(mem_r, NULL);
		} /* else null union init */

		asm_declare_pad(f,
				type_ref_size(r, NULL) - sub,
				"union extra");

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
		asm_declare_init_type(f, tfor);
		static_addr(exp);
		fputc('\n', f);
	}
}

static void asm_nam_begin(FILE *f, decl *d)
{
	fprintf(f,
			".align %u\n"
			"%s:\n",
			decl_align(d),
			decl_asm_spel(d));
}

static void asm_reserve_bytes(unsigned nbytes)
{
	/*
	 * TODO: .comm buf,512,5
	 * or    .zerofill SECTION_NAME,buf,512,5
	 */
	asm_out_section(SECTION_BSS, ".space %u\n", nbytes);
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
		asm_nam_begin(f, d);
		asm_declare_init(f, d->init, d->ref);
		fputc('\n', f);

	}else{
		/* always resB, since we use decl_size() */
		asm_nam_begin(cc_out[SECTION_BSS], d);
		asm_reserve_bytes(decl_size(d));
	}
}

void asm_out_section(enum section_type t, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(cc_out[t], fmt, l);
	va_end(l);
}
