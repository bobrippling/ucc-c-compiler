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
#include "../pack.h"

#define ASSERT_SCALAR(di)                  \
	UCC_ASSERT(di->type == decl_init_scalar, \
			"scalar expected for bitfield init")

#define ASM_COMMENT "#"

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
		fprintf(f, ".space %u " ASM_COMMENT " %s\n", pad, why);
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
#define BITFIELD_DBG(...) /*fprintf(stderr, __VA_ARGS__)*/
	intval_t v = 0;
	unsigned width = 0;
	unsigned i;

	BITFIELD_DBG("bitfield out -- new\n");
	for(i = 0; i < n; i++){
		intval_t this = intval_truncate_bits(
				vals[i].val, vals[i].width, NULL);

		width += vals[i].width;

		BITFIELD_DBG("bitfield out: 0x%llx << %u gives ",
				this, vals[i].offset);

		v |= this << vals[i].offset;

		BITFIELD_DBG("0x%llx\n", v);
	}

	BITFIELD_DBG("bitfield done with 0x%llx\n", v);

	if(width > 0){
		asm_declare_init_type(f, ty);
		fprintf(f, "%" INTVAL_FMT_D "\n", v);
	}else{
		fprintf(f, ASM_COMMENT " skipping zero length bitfield%s init\n",
				n == 1 ? "" : "s");
	}
}

static void bitfields_out(
		FILE *f,
		struct bitfield_val *bfs, unsigned *pn,
		type_ref *ty)
{
	asm_declare_init_bitfields(f, bfs, *pn, ty);
	*pn = 0;
}

static void bitfield_val_set(
		struct bitfield_val *bfv, expr *kval, expr *field_w)
{
	bfv->val = kval ? const_fold_val(kval) : 0;
	bfv->offset = 0;
	bfv->width = const_fold_val(field_w);
}

static struct bitfield_val *bitfields_add(
		struct bitfield_val *bfs, unsigned *pn,
		decl *mem, decl_init *di)
{
	const unsigned i = *pn;

	bfs = urealloc1(bfs, (*pn += 1) * sizeof *bfs);

	if(di)
		ASSERT_SCALAR(di);

	bitfield_val_set(&bfs[i],
			di ? di->bits.expr : NULL,
			mem->field_width);

	bfs[i].offset = mem->struct_offset_bitfield;

	return bfs;
}

static void asm_declare_init(FILE *f, decl_init *init, type_ref *tfor)
{
	type_ref *r;

	if(init == DYNARRAY_NULL)
		init = NULL;

	if(!init){
		/* don't initialise flex-arrays */
		if(!type_ref_is_incomplete_array(tfor)){
			asm_declare_pad(f, type_ref_size(tfor, NULL),
					"null init"/*, type_ref_to_str(tfor)*/);
		}else{
			fprintf(f, ASM_COMMENT " flex array init skipped\n");
		}

	}else if((r = type_ref_is_type(tfor, type_struct))){
		/* array of stmts for each member
		 * assumes the ->bits.inits order is member order
		 */
		struct_union_enum_st *const sue = r->bits.type->sue;
		sue_member **mem;
		decl_init **i;
		unsigned end_of_last = 0;
		struct bitfield_val *bitfields = NULL;
		unsigned nbitfields = 0;
		decl *first_bf = NULL;

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");

#define DEBUG(s, ...) /*fprintf(f, "\033[35m" s "\033[m\n", __VA_ARGS__)*/

		i = init->bits.ar.inits;
		/* iterate using members, not inits */
		for(mem = sue->members;
				mem && *mem;
				mem++)
		{
			decl *d_mem = (*mem)->struct_member;
			decl_init *di_to_use = NULL;

			if(i){
				int inc = 1;

				if(*i == NULL)
					inc = 0;
				else if(*i != DYNARRAY_NULL)
					di_to_use = *i;

				if(inc){
					i++;
					if(!*i)
						i = NULL; /* reached end */
				}
			}

			DEBUG("init for %ld/%s, %s",
					mem - sue->members, d_mem->spel,
					di_to_use ? di_to_use->bits.expr->f_str() : NULL);

			/* only pad if we're not on a bitfield or we're on the first bitfield */
			if(!d_mem->field_width || !first_bf){
				DEBUG("prev padding, offset=%d, end_of_last=%d",
						d_mem->struct_offset, end_of_last);

				UCC_ASSERT(
						d_mem->struct_offset >= end_of_last,
						"negative struct pad, sue %s, member %s "
						"offset %u, end_of_last %u",
						sue->spel, decl_to_str(d_mem),
						d_mem->struct_offset, end_of_last);

				asm_declare_pad(f,
						d_mem->struct_offset - end_of_last,
						"prev struct padding");
			}

			if(d_mem->field_width){
				if(!first_bf || d_mem->first_bitfield){
					if(first_bf){
						DEBUG("new bitfield group (%s is new boundary), old:",
								d_mem->spel);
						/* next bitfield group - store the current */
						bitfields_out(f, bitfields, &nbitfields, first_bf->ref);
					}
					first_bf = d_mem;
				}

				bitfields = bitfields_add(
						bitfields, &nbitfields,
						d_mem, di_to_use);

			}else{
				if(nbitfields){
					DEBUG("at non-bitfield, prev-bitfield out:", 0);
					bitfields_out(f, bitfields, &nbitfields, first_bf->ref);
					first_bf = NULL;
				}

				DEBUG("normal init for %s:", d_mem->spel);
				asm_declare_init(f, di_to_use, d_mem->ref);
			}

			if(type_ref_is_incomplete_array(d_mem->ref)){
				UCC_ASSERT(!mem[1], "flex-arr not at end");
			}else if(!d_mem->field_width || d_mem->first_bitfield){
				unsigned last_sz = type_ref_size(d_mem->ref, NULL);

				end_of_last = d_mem->struct_offset + last_sz;
				DEBUG("done with member \"%s\", end_of_last = %d",
						d_mem->spel, end_of_last);
			}
		}

		if(nbitfields)
			bitfields_out(f, bitfields, &nbitfields, first_bf->ref);
		free(bitfields);

		/* need to pad to struct size */
		asm_declare_pad(f,
				sue_size(sue, NULL) - end_of_last,
				"struct tail");

	}else if((r = type_ref_is(tfor, type_ref_array))){
		size_t i, len;
		decl_init **p;
		type_ref *next = type_ref_next(tfor);

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");

		if(type_ref_is_incomplete_array(tfor)){
			len = dynarray_count(init->bits.ar.inits);
		}else{
			UCC_ASSERT(type_ref_is_complete(tfor), "incomplete array/type init");
			len = type_ref_array_len(tfor);
		}

		for(i = len, p = init->bits.ar.inits;
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

				bitfield_val_set(&bfv, u_init->bits.expr, mem->field_width);

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
		DEBUG("  scalar init for %s:", type_ref_to_str(tfor));
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
	asm_declare_pad(cc_out[SECTION_BSS], nbytes, "object space");
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
