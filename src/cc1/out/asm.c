#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "../../util/util.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../../util/math.h"
#include "../../util/io.h"

#include "../type.h"
#include "../type_nav.h"
#include "../decl.h"
#include "../strings.h"

#include "asm.h"
#include "out.h"
#include "impl_fp.h"

#include "../fopt.h"
#include "../cc1.h"
#include "../sym.h"
#include "../expr.h"
#include "../stmt.h"
#include "../type_is.h"

#include "../sue.h"
#include "../const.h"
#include "../gen_asm.h"
#include "../decl_init.h"
#include "../pack.h"
#include "../str.h"
#include "../cc1_target.h"
#include "../cc1_out.h"
#include "../mangle.h"

#include "../ops/expr_compound_lit.h"

#define ASSERT_SCALAR(di)                  \
	UCC_ASSERT(di->type == decl_init_scalar, \
			"scalar expected for bitfield init")

#define ASM_COMMENT "#"

#define INIT_DEBUG 0

#define DEBUG(s, ...) do{ \
	if(INIT_DEBUG) fprintf(stderr, "\033[35m" s "\033[m\n", __VA_ARGS__); \
}while(0)

struct bitfield_val
{
	integral_t val;
	unsigned offset;
	unsigned width;
};

const char *asm_section_desc(enum section_builtin sec)
{
	switch(sec){
		case SECTION_TEXT: return SECTION_DESC_TEXT;
		case SECTION_DATA: return SECTION_DESC_DATA;
		case SECTION_BSS: return SECTION_DESC_BSS;
		case SECTION_RODATA: return SECTION_DESC_RODATA;
		case SECTION_RELRO: return SECTION_DESC_RELRO;
		case SECTION_CTORS: return SECTION_DESC_CTORS;
		case SECTION_DTORS: return SECTION_DESC_DTORS;
		case SECTION_DBG_ABBREV: return SECTION_DESC_DBG_ABBREV;
		case SECTION_DBG_INFO: return SECTION_DESC_DBG_INFO;
		case SECTION_DBG_LINE: return SECTION_DESC_DBG_LINE;
	}
	return NULL;
}

static void switch_section_emit(const struct section *section)
{
	const char *desc = NULL;
	char *name;
	int allocated;
	const int is_builtin = section_is_builtin(section);
	int firsttime;

	if(is_builtin)
		desc = asm_section_desc(section->builtin);

	name = section_name(section, &allocated);
	xfprintf(cc1_output.file, ".section %s", name);
	if(allocated)
		free(name), name = NULL;

	firsttime = cc1_outsections_add(section);
	if(firsttime){
		if(cc1_target_details.as->supports_section_flags && !is_builtin){
			const int is_code = section->flags & SECTION_FLAG_EXECUTABLE;
			const int is_rw = !(section->flags & SECTION_FLAG_RO);

			xfprintf(cc1_output.file, ",\"a%s\",@progbits", is_code ? "x" : is_rw ? "w" : "");
		}
	}
	xfprintf(cc1_output.file, "\n");

	if(firsttime && desc)
		xfprintf(cc1_output.file, "%s%s%s:\n", cc1_target_details.as->privatelbl_prefix, SECTION_BEGIN, desc);
}

void asm_switch_section(const struct section *section)
{
	if(cc1_output.section.builtin != -1
	&& section_eq(&cc1_output.section, section))
	{
		return;
	}

	memcpy_safe(&cc1_output.section, section);

	switch_section_emit(section);
}

int asm_table_lookup(type *r)
{
	int sz;
	int i;

	if(!r)
		sz = type_primitive_size(type_long); /* or ptr */
	else if(type_is(r, type_array) || type_is(r, type_func))
		/* special case for funcs and arrays */
		sz = platform_word_size();
	else
		sz = type_size(r, NULL);

	for(i = 0; i < ASM_TABLE_LEN; i++)
		if(asm_type_table[i].sz == sz)
			return i;

	ICE("no asm type index for byte size %d", sz);
	return -1;
}

const char *asm_type_directive(type *r)
{
	return asm_type_table[asm_table_lookup(r)].directive;
}

int asm_type_size(type *r)
{
	return asm_type_table[asm_table_lookup(r)].sz;
}

static void asm_declare_pad(const struct section *sec, unsigned pad, const char *why)
{
	if(pad)
		asm_out_section(sec, ".space %u " ASM_COMMENT " %s\n", pad, why);
}

static void asm_declare_init_type(const struct section *sec, type *ty)
{
	asm_out_section(sec, ".%s ", asm_type_directive(ty));
}

static void asm_declare_init_bitfields(
		const struct section *sec,
		struct bitfield_val *vals, unsigned n,
		type *ty)
{
#define BITFIELD_DBG(...) /*fprintf(stderr, __VA_ARGS__)*/
	integral_t v = 0;
	unsigned width = 0;
	unsigned i;

	BITFIELD_DBG("bitfield out -- new\n");
	for(i = 0; i < n; i++){
		integral_t this = integral_truncate_bits(
				vals[i].val, vals[i].width, NULL);

		width += vals[i].width;

		BITFIELD_DBG("bitfield out: 0x%llx << %u gives ",
				this, vals[i].offset);

		v |= this << vals[i].offset;

		BITFIELD_DBG("0x%llx\n", v);
	}

	BITFIELD_DBG("bitfield done with 0x%llx\n", v);

	if(width > 0){
		asm_declare_init_type(sec, ty);
		asm_out_section(sec, "%" NUMERIC_FMT_D "\n", v);
	}else{
		asm_out_section(sec,
				ASM_COMMENT " skipping zero length bitfield%s init\n",
				n == 1 ? "" : "s");
	}
}

static void bitfields_out(
		const struct section *sec,
		struct bitfield_val *bfs, unsigned *pn,
		type *ty)
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
			mem->bits.var.field_width);

	bfs[i].offset = mem->bits.var.struct_offset_bitfield;

	return bfs;
}

void asm_out_fp(const struct section *sec, type *ty, floating_t f)
{
	const enum type_primitive prim = type_primitive(ty);
	char buf[sizeof(long double)] = { 0 };

	impl_fp_bits(buf, sizeof(buf), prim, f);

	switch(prim){
		case type_float:
		{
			unsigned u;
			UCC_STATIC_ASSERT(sizeof(float) == sizeof(u));

			memcpy(&u, buf, sizeof(u));

			asm_out_section(sec, ".long %u # float %f\n", u, (float)f);
			break;
		}

		case type_double:
		{
			unsigned long long ul;
			UCC_STATIC_ASSERT(sizeof(double) == sizeof(ul));

			memcpy(&ul, buf, sizeof(ul));

			asm_out_section(sec, ".quad %llu # double %f\n", ul, (double)f);
			break;
		}

		case type_ldouble:
			ICE("TODO");
		default:
			ICE("bad float type");
	}
}

static void static_val(const struct section *sec, type *ty, expr *e)
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
				/* use 'ty' here - e->tree_type will be the casted-from type,
				 * e.g.
				 * char x[] = { 5ull };
				 * we want 'char', not 'unsigned long long' */
				integral_str(buf, sizeof buf, k.bits.num.val.i, ty);
				asm_out_section(sec, "%s", buf);
			}
			break;

		case CONST_ADDR:
			asm_declare_init_type(sec, ty);
			switch(k.bits.addr.lbl_type){
				case CONST_LBL_TRUE:
				case CONST_LBL_WEAK:
					asm_out_section(sec, "%s", k.bits.addr.bits.lbl);
					break;
				case CONST_LBL_MEMADDR:
					asm_out_section(sec, "%ld", k.bits.addr.bits.memaddr);
					break;
			}
			break;

		case CONST_STRK:
			stringlit_use(k.bits.str->lit); /* must be before the label access */
			asm_declare_init_type(sec, ty);
			asm_out_section(sec, "%s", k.bits.str->lit->lbl);
			break;
	}

	/* offset in bytes, no mul needed */
	if(k.offset)
		asm_out_section(sec, " + %" NUMERIC_FMT_D, k.offset);
	asm_out_section(sec, "\n");
}

static void asm_declare_init(const struct section *sec, decl_init *init, type *tfor)
{
	type *r;

	if(init == DYNARRAY_NULL)
		init = NULL;

	if(!init){
		/* don't initialise flex-arrays */
		if(!type_is_incomplete_array(tfor)){
			asm_declare_pad(sec, type_size(tfor, NULL),
					"null init"/*, type_to_str(tfor)*/);
		}else{
			asm_out_section(sec, ASM_COMMENT " flex array init skipped\n");
		}

	}else if((r = type_is_primitive(tfor, type_struct))){
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
		expr *copy_from_exp;

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");

		i = init->bits.ar.inits;

		/* check for compound-literal copy-init */
		if((copy_from_exp = decl_init_is_struct_copy(init, sue))){
			decl_init *copy_from_init;

			copy_from_exp = expr_skip_lval2rval(copy_from_exp);

			/* the only struct-expression that's possible
			 * in static context is a compound literal */
			assert(expr_kind(copy_from_exp, compound_lit)
					&& "unhandled expression init");

			copy_from_init = expr_comp_lit_init(copy_from_exp);
			assert(copy_from_init->type == decl_init_brace);

			i = copy_from_init->bits.ar.inits;
		}

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
					di_to_use && di_to_use->type == decl_init_scalar
					? di_to_use->bits.expr->f_str()
					: NULL);

			/* only pad if we're not on a bitfield or we're on the first bitfield */
			if(!d_mem->bits.var.field_width || !first_bf){
				DEBUG("prev padding, offset=%d, end_of_last=%d",
						d_mem->bits.var.struct_offset, end_of_last);

				UCC_ASSERT(
						d_mem->bits.var.struct_offset >= end_of_last,
						"negative struct pad, %s::%s @ %u >= end_of_last @ %u",
						sue->spel, decl_to_str(d_mem),
						d_mem->bits.var.struct_offset, end_of_last);

				asm_declare_pad(sec,
						d_mem->bits.var.struct_offset - end_of_last,
						"prev struct padding");
			}

			if(d_mem->bits.var.field_width){
				const int zero_width = const_fold_val_i(d_mem->bits.var.field_width) == 0;

				if(!first_bf || d_mem->bits.var.first_bitfield){
					if(first_bf){
						DEBUG("new bitfield group (%s is new boundary), old:",
								d_mem->spel);
						/* next bitfield group - store the current */
						bitfields_out(sec, bitfields, &nbitfields, first_bf->ref);
					}
					if(!zero_width)
						first_bf = d_mem;
				}

				if(!zero_width){
					bitfields = bitfields_add(
							bitfields, &nbitfields,
							d_mem, di_to_use);
				}

			}else{
				if(nbitfields){
					DEBUG("at non-bitfield, prev-bitfield out:", 0);

					bitfields_out(sec, bitfields, &nbitfields, decl_type_for_bitfield(first_bf));
					first_bf = NULL;
				}

				DEBUG("normal init for %s:", d_mem->spel);
				asm_declare_init(sec, di_to_use, d_mem->ref);
			}

			if(type_is_incomplete_array(d_mem->ref)){
				UCC_ASSERT(!mem[1], "flex-arr not at end");
			}else if(!d_mem->bits.var.field_width || d_mem->bits.var.first_bitfield){
				unsigned sz, align;

				decl_size_align_inc_bitfield(d_mem, &sz, &align);

				end_of_last = d_mem->bits.var.struct_offset + sz;
				DEBUG("done with member \"%s\", end_of_last = %d",
						d_mem->spel, end_of_last);
			}
		}

		if(nbitfields)
			bitfields_out(sec, bitfields, &nbitfields, first_bf->ref);
		free(bitfields);

		/* need to pad to struct size */
		asm_declare_pad(sec,
				sue_size(sue, NULL) - end_of_last,
				"struct tail");

	}else if((r = type_is(tfor, type_array))){
		size_t i, len;
		decl_init **p;
		type *next = type_next(tfor);

		UCC_ASSERT(init->type == decl_init_brace, "unbraced struct");

		if(type_is_incomplete_array(tfor)){
			len = dynarray_count(init->bits.ar.inits);
		}else{
			UCC_ASSERT(type_is_complete(tfor), "incomplete array/type init");
			len = type_array_len(tfor);
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

	}else if((r = type_is_primitive(tfor, type_union))){
		/* union inits are decl_init_brace with spaces up to the first union init,
		 * then NULL/end of the init-array */
		struct_union_enum_st *sue = type_is_s_or_u(r);
		unsigned i, sub = 0;
		decl_init *u_init;

		UCC_ASSERT(init->type == decl_init_brace, "brace init expected");

		/* skip the empties until we get to one */
		for(i = 0; init->bits.ar.inits[i] == DYNARRAY_NULL; i++);

		if((u_init = init->bits.ar.inits[i])){
			decl *mem = sue->members[i]->struct_member;
			type *mem_r = mem->ref;

			/* union init, member at index `i' */
			if(mem->bits.var.field_width){
				/* we know it's integral */
				struct bitfield_val bfv;

				ASSERT_SCALAR(u_init);

				bitfield_val_set(&bfv, u_init->bits.expr, mem->bits.var.field_width);

				asm_declare_init_bitfields(sec, &bfv, 1, mem_r);
			}else{
				asm_declare_init(sec, u_init, mem_r);
			}

			sub = type_size(mem_r, NULL);
		} /* else null union init */

		asm_declare_pad(sec,
				type_size(r, NULL) - sub,
				"union extra");

	}else{
		/* scalar */
		expr *exp = init->bits.expr;

		UCC_ASSERT(init->type == decl_init_scalar, "scalar init expected");

		/* exp->tree_type should match tfor */
		{
			char buf[TYPE_STATIC_BUFSIZ];

			UCC_ASSERT(
					type_cmp(exp->tree_type, tfor, TYPE_CMP_ALLOW_TENATIVE_ARRAY) != TYPE_NOT_EQUAL,
					"mismatching init types: %s and %s",
					type_to_str_r(buf, exp->tree_type),
					type_to_str(tfor));
		}

		/* use tfor, since "abc" has type (char[]){(int)'a', (int)'b', ...} */
		DEBUG("  scalar init for %s:", type_to_str(tfor));
		static_val(sec, tfor, exp);
	}
}

void asm_out_align(const struct section *sec, unsigned align)
{
	if(mopt_mode & MOPT_ALIGN_IS_POW2)
		align = log2i(align);

	if(align)
		asm_out_section(sec, ".align %u\n", align);
}

void asm_nam_begin3(const struct section *sec, const char *lbl, unsigned align)
{
	asm_out_align(sec, align);
	asm_out_section(sec, "%s:\n", lbl);
}

static void asm_nam_begin(const struct section *sec, decl *d)
{
	asm_nam_begin3(sec, decl_asm_spel(d), decl_align(d));
}

static void asm_reserve_bytes(const struct section *sec, unsigned nbytes)
{
	/*
	 * TODO: .comm buf,512,5
	 * or    .zerofill SECTION_NAME,buf,512,5
	 */
	asm_declare_pad(sec, nbytes, "object space");
}

static void asm_predecl(const struct section *sec, const char *type, decl *d)
{
	asm_out_section(sec, ".%s %s\n", type, decl_asm_spel(d));
}

void asm_predeclare_extern(const struct section *sec, decl *d)
{
	(void)sec;
	(void)d;
	/*
	asm_comment("extern %s", d->spel);
	asm_out_section(SECTION_BSS, "extern %s", d->spel);
	*/
}

void asm_predeclare_global(const struct section *sec, decl *d)
{
	asm_predecl(sec, "globl", d);
}

void asm_predeclare_used(const struct section *sec, decl *d)
{
	if(cc1_target_details.as->directives.no_dead_strip)
		asm_predecl(sec, cc1_target_details.as->directives.no_dead_strip, d);
}

void asm_predeclare_weak(const struct section *sec, decl *d)
{
	asm_predecl(sec, cc1_target_details.as->directives.weak, d);
}

void asm_declare_alias(const struct section *sec, decl *d, decl *alias)
{
	asm_out_section(sec, "%s = %s\n", decl_asm_spel(d), decl_asm_spel(alias));
}

void asm_predeclare_visibility(const struct section *sec, decl *d)
{
	if(decl_linkage(d) == linkage_internal)
		return;

	switch(decl_visibility(d)){
		case VISIBILITY_DEFAULT:
			break;
		case VISIBILITY_HIDDEN:
			asm_predecl(sec, cc1_target_details.as->directives.visibility_hidden, d);
			break;
		case VISIBILITY_PROTECTED:
			assert(cc1_target_details.as->supports_visibility_protected);
			asm_predecl(sec, "protected", d);
			break;
	}
}

static void asm_declare_ctor_dtor(decl *d, enum section_builtin sec)
{
	const struct section section = SECTION_INIT(sec);
	type *intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);
	const char *directive = asm_type_directive(intptr_ty);

	/*if(asm_section_empty(sec))
		asm_out_align(sec, type_align(intptr_ty, NULL));

		// should be aligned by the linker, the above should be a no-op
	*/
	asm_out_section(&section, ".%s %s\n", directive, decl_asm_spel(d));
}

void asm_declare_constructor(decl *d)
{
	asm_declare_ctor_dtor(d, SECTION_CTORS);
}

void asm_declare_destructor(decl *d)
{
	asm_declare_ctor_dtor(d, SECTION_DTORS);
}

void asm_declare_stringlit(const struct section *sec, const stringlit *lit)
{
	/* could be SECTION_RODATA */
	asm_nam_begin3(sec, lit->lbl, /*align:*/1);

	switch(lit->cstr->type){
		case CSTRING_WIDE:
		{
			const char *join = "";
			size_t i;
			asm_out_section(sec, ".long ");
			for(i = 0; i < lit->cstr->count; i++){
				asm_out_section(sec, "%s%d", join, lit->cstr->bits.wides[i]);
				join = ", ";
			}
			break;
		}

		case CSTRING_RAW:
			assert(0 && "raw string in code gen");

		case CSTRING_ASCII:
		{
			FILE *f = cc1_output.file;
			asm_out_section(sec, ".ascii \"");
			literal_print(f, lit->cstr);
			fputc('"', f);
			break;
		}
	}

	asm_out_section(sec, "\n");
}

void asm_declare_decl_init(const struct section *sec, decl *d)
{
	if((d->store & STORE_MASK_STORE) == store_extern){
		asm_predeclare_extern(sec, d);
		return;
	}

	/* d->bits.var.init.dinit may be null for extern decls */
	if(d->bits.var.init.dinit){
		int nonzero_init = !DECL_INIT_COMPILER_GENERATED(d->bits.var.init)
			&& !decl_init_is_zero(d->bits.var.init.dinit);

		if(nonzero_init){
			asm_nam_begin(sec, d);
			asm_declare_init(sec, d->bits.var.init.dinit, d->ref);
			asm_out_section(sec, "\n");
			return;
		}
	}

	if(section_is_builtin(sec)
	&& DECL_INIT_COMPILER_GENERATED(d->bits.var.init)
	&& cc1_fopt.common
	&& !attribute_present(d, attr_weak) /* variables can't be weak and common */)
	{
		unsigned align;

		if(decl_linkage(d) == linkage_internal){
			if(!cc1_target_details.as->supports_local_common)
				goto fallback;

			asm_out_section(sec, ".local %s\n", decl_asm_spel(d));
		}

		align = decl_align(d);
		if(mopt_mode & MOPT_ALIGN_IS_POW2){
			align = log2i(align);
		}

		asm_out_section(sec, ".comm %s,%u,%u\n",
				decl_asm_spel(d), decl_size(d), align);
		return;
	}

fallback:
	/* always resB, since we use decl_size() */
	asm_nam_begin(sec, d);
	asm_reserve_bytes(sec, decl_size(d));
}

void asm_out_sectionv(const struct section *sec, const char *fmt, va_list l)
{
	asm_switch_section(sec);

	vfprintf(cc1_output.file, fmt, l);
}

void asm_out_section(const struct section *sec, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	asm_out_sectionv(sec, fmt, l);
	va_end(l);
}
