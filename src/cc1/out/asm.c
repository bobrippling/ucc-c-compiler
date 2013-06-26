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
#include "out.h"

#define ASSERT_SCALAR(di)                  \
	UCC_ASSERT(di->type == decl_init_scalar, \
			"scalar expected for bitfield init")

struct bitfield_val
{
	integral_t val;
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

const char *asm_type_directive(type_ref *r)
{
	return asm_type_table[asm_table_lookup(r)].directive;
}

int asm_type_size(type_ref *r)
{
	return asm_type_table[asm_table_lookup(r)].sz;
}

static void asm_declare_pad(enum section_type sec, unsigned pad, const char *why)
{
	if(pad)
		asm_out_section(sec, ".space %u # %s\n", pad, why);
}

static void asm_declare_init_type(enum section_type sec, type_ref *ty)
{
	asm_out_section(sec, ".%s ", asm_type_directive(ty));
}

static void asm_declare_init_bitfields(
		enum section_type sec,
		struct bitfield_val *vals, unsigned n,
		type_ref *ty)
{
#define BITFIELD_DBG(...) /*fprintf(stderr, __VA_ARGS__)*/
	integral_t v = 0;
	unsigned i;

	BITFIELD_DBG("bitfield out -- new\n");
	for(i = 0; i < n; i++){
		integral_t this = integral_truncate_bits(
				vals[i].val, vals[i].width);

		BITFIELD_DBG("bitfield out: 0x%llx << %u gives ",
				this, vals[i].offset);

		v |= this << vals[i].offset;

		BITFIELD_DBG("0x%llx\n", v);
	}

	BITFIELD_DBG("bitfield done with 0x%llx\n", v);

	asm_declare_init_type(sec, ty);
	asm_out_section(sec, "%" NUMERIC_FMT_D "\n", v);
}

static void bitfields_out(
		enum section_type sec,
		struct bitfield_val *bfs, unsigned *pn,
		type_ref *ty)
{
	asm_declare_init_bitfields(sec, bfs, *pn, ty);
	*pn = 0;
}

static void bitfield_val_set(
		struct bitfield_val *bfv, expr *kval, expr *field_w)
{
	bfv->val = kval ? const_fold_val_i(kval) : 0;
	bfv->offset = 0;
	bfv->width = const_fold_val_i(field_w);
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

void asm_out_fp(enum section_type sec, type_ref *ty, floating_t f)
{
	switch(type_ref_primitive(ty)){
		case type_float:
			{
				union { float f; unsigned u; } u;
				u.f = f;
				asm_out_section(sec, ".long %u\n", u.u);
				out_comment_sec(sec, "float %f", u.f);
				break;
			}

		case type_double:
			{
				union { double d; unsigned long ul; } u;
				u.d = f;
				asm_out_section(sec, ".quad %lu\n", u.ul);
				out_comment_sec(sec, "double %f", u.d);
				break;
			}
		case type_ldouble:
			ICE("TODO");
		default:
			ICE("bad float type");
	}
}

static void static_val(enum section_type sec, type_ref *ty, expr *e)
{
	consty k;

	memset(&k, 0, sizeof k);

	const_fold(e, &k);

	switch(k.type){
		case CONST_NEED_ADDR:
		case CONST_NO:
			ICE("non-constant expr-%s const=%d%s",
					e->f_str(),
					k.type,
					k.type == CONST_NEED_ADDR ? " (needs addr)" : "");
			break;

		case CONST_NUM:
			if(K_FLOATING(k.bits.num)){
				/* asm fp const */
				asm_out_fp(sec, ty, k.bits.num.val.f);
			}else{
				char buf[INTEGRAL_BUF_SIZ];
				asm_declare_init_type(sec, ty);
				integral_str(buf, sizeof buf, k.bits.num.val.i, e->tree_type);
				asm_out_section(sec, "%s", buf);
			}
			break;

		case CONST_ADDR:
			asm_declare_init_type(sec, ty);
			if(k.bits.addr.is_lbl)
				asm_out_section(sec, "%s", k.bits.addr.bits.lbl);
			else
				asm_out_section(sec, "%ld", k.bits.addr.bits.memaddr);
			break;

		case CONST_STRK:
			asm_declare_init_type(sec, ty);
			asm_out_section(sec, "%s", k.bits.str->lbl);
			break;
	}

	/* offset in bytes, no mul needed */
	if(k.offset)
		asm_out_section(sec, " + %ld", k.offset);
	asm_out_section(sec, "\n");
}

static void asm_declare_init(enum section_type sec, decl_init *init, type_ref *tfor)
{
	type_ref *r;

	if(init == DYNARRAY_NULL)
		init = NULL;

	if(!init){
		/* don't initialise flex-arrays */
		if(!type_ref_is_incomplete_array(tfor)){
			asm_declare_pad(sec, type_ref_size(tfor, NULL),
					"null init"/*, type_ref_to_str(tfor)*/);
		}else{
			asm_out_section(sec, "# flex array init skipped\n");
		}

	}else if((r = type_ref_is_type(tfor, type_struct))){
		/* array of stmts for each member
		 * assumes the ->bits.inits order is member order
		 */
		struct_union_enum_st *const sue = r->bits.type->sue;
		sue_member **mem;
		decl_init **i;
		int end_of_last = 0;
		struct bitfield_val *bitfields = NULL;
		unsigned nbitfields = 0;
		decl *first_bf = NULL;

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");
		i = init->bits.ar.inits;

		/* iterate using members, not inits */
		for(mem = sue->members;
				mem && *mem;
				mem++)
		{
			decl *d_mem = (*mem)->struct_member;
			int inc_iter = 1;

			/* only pad if we're not on a bitfield or we're on the first bitfield */
			if(!d_mem->field_width || !first_bf)
				asm_declare_pad(sec, d_mem->struct_offset - end_of_last, "struct padding");

			if(d_mem->field_width){
				decl_init *di_to_use = NULL;

				if(!first_bf || d_mem->first_bitfield){
					if(first_bf){
						/* next bitfield group - store the current */
						bitfields_out(sec, bitfields, &nbitfields, first_bf->ref);
					}
					first_bf = d_mem;
				}

				if(d_mem->spel && i){
					if((di_to_use = *i) == DYNARRAY_NULL)
						di_to_use = NULL;
				}else{
					inc_iter = 0;
				}

				bitfields = bitfields_add(
						bitfields, &nbitfields,
						d_mem, di_to_use);

			}else{
				if(nbitfields){
					bitfields_out(sec, bitfields, &nbitfields, first_bf->ref);
					first_bf = NULL;
				}

				asm_declare_init(sec, i ? *i : NULL, d_mem->ref);
			}

			if(inc_iter && i && !*++i)
				i = NULL; /* reached end */

			if(type_ref_is_incomplete_array(d_mem->ref)){
				UCC_ASSERT(!mem[1], "flex-arr not at end");
			}else{
				unsigned last_sz = type_ref_size(d_mem->ref, NULL);
				end_of_last = d_mem->struct_offset + last_sz;
			}
		}

		if(nbitfields)
			bitfields_out(sec, bitfields, &nbitfields, first_bf->ref);
		free(bitfields);

		/* need to pad to struct size */
		asm_declare_pad(sec,
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

			asm_declare_init(sec, this, next);
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

				asm_declare_init_bitfields(sec, &bfv, 1, mem_r);
			}else{
				asm_declare_init(sec, u_init, mem_r);
			}

			sub = type_ref_size(mem_r, NULL);
		} /* else null union init */

		asm_declare_pad(sec,
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
		static_val(sec, tfor, exp);
	}
}

void asm_label(enum section_type sec, const char *lbl, unsigned align)
{
	asm_out_section(sec,
			".align %u\n"
			"%s:\n",
			align, lbl);
}

static void asm_reserve_bytes(enum section_type sec, unsigned nbytes)
{
	/*
	 * TODO: .comm buf,512,5
	 * or    .zerofill SECTION_NAME,buf,512,5
	 */
	asm_out_section(sec, ".space %u\n", nbytes);
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

void asm_declare_decl_init(enum section_type sec, decl *d)
{
	if((d->store & STORE_MASK_STORE) == store_extern){
		asm_predeclare_extern(d);

	}else if(d->init && !decl_init_is_zero(d->init)){
		asm_label(sec, decl_asm_spel(d), decl_align(d));
		asm_declare_init(sec, d->init, d->ref);
		asm_out_section(sec, "\n");

	}else{
		/* always resB, since we use decl_size() */
		asm_label(SECTION_BSS, decl_asm_spel(d), decl_align(d));
		asm_reserve_bytes(SECTION_BSS, decl_size(d));
	}
}

void asm_out_sectionv(enum section_type t, const char *fmt, va_list l)
{
	vfprintf(cc_out[t], fmt, l);
}

void asm_out_section(enum section_type t, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	asm_out_sectionv(t, fmt, l);
	va_end(l);
}
