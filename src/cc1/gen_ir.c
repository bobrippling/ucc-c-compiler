#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <strbuf_fixed.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "../util/math.h"
#include "../util/platform.h"

#include "sym.h"
#include "type_is.h"
#include "type_nav.h"
#include "expr.h"
#include "stmt.h"
#include "cc1.h" /* IS_32_BIT() */
#include "sue.h"
#include "funcargs.h"
#include "str.h" /* literal_print() */
#include "decl_init.h"
#include "bitfields.h"
#include "fold.h"
#include "pack.h"

#include "gen_ir.h"
#include "gen_ir_internal.h"

#define IR_DUMP_FIELD_OFFSETS 1

struct irval
{
	enum irval_type
	{
		IRVAL_LITERAL,
		IRVAL_ID,
		IRVAL_NAMED,
		IRVAL_LBL
	} type;

	union
	{
		struct
		{
			type *ty;
			integral_t val;
		} lit;
		irid id;
		decl *decl;
		const char *lbl;
	} bits;
};

enum ir_comment
{
	IR_COMMENT_CHAR = 1 << 0,
	IR_COMMENT_NEWLINE = 1 << 1
};

static void gen_ir_init_r(irctx *, decl_init *init, type *ty);

static const char *irtype_str_maybe_fn(type *t, funcargs *args, irctx *ctx);
static unsigned irtype_struct_num(irctx *ctx, struct_union_enum_st *su);
static const char *irtype_su_str_full(struct_union_enum_st *su, irctx *);
static const char *irtype_su_str_abbreviated(struct_union_enum_st *su, irctx *ctx);

irval *gen_ir_expr(const struct expr *expr, irctx *ctx)
{
	return expr->f_ir(expr, ctx);
}

void gen_ir_stmt(const struct stmt *stmt, irctx *ctx)
{
	stmt->f_ir(stmt, ctx);
}

static void gen_ir_comment_opts_v(
		enum ir_comment opts, irctx *ctx, const char *fmt, va_list l)
{
	(void)ctx;
	if(opts & IR_COMMENT_CHAR)
		printf("# ");
	vprintf(fmt, l);
	if(opts & IR_COMMENT_NEWLINE)
		putchar('\n');
}

static void gen_ir_comment_opts(
		enum ir_comment opts, irctx *ctx, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	gen_ir_comment_opts_v(opts, ctx, fmt, l);
	va_end(l);
}

void gen_ir_comment(irctx *ctx, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	gen_ir_comment_opts_v(IR_COMMENT_CHAR | IR_COMMENT_NEWLINE, ctx, fmt, l);
	va_end(l);
}

static void gen_ir_spill_args(irctx *ctx, funcargs *args)
{
	decl **i;

	(void)ctx;

	for(i = args->arglist; i && *i; i++){
		decl *d = *i;
		const char *asm_spel = decl_asm_spel(d);

		printf("$%s = alloca %s\n", asm_spel, irtype_str(d->ref, ctx));
		printf("store $%s, $%s\n", asm_spel, d->spel);
	}
}

static void gen_ir_integral(integral_t i, type *ty)
{
	char buf[INTEGRAL_BUF_SIZ];
	integral_str(buf, sizeof buf, i, ty);
	fputs(buf, stdout);
}

static void gen_ir_init_scalar(decl_init *init)
{
	int anyptr = 0;
	expr *e = init->bits.expr;
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
				IRTODO("floating static constant");
				printf("<TODO: float constant>");
			}else{
				gen_ir_integral(k.bits.num.val.i, e->tree_type);
			}
			break;

		case CONST_ADDR:
			if(k.bits.addr.is_lbl)
				printf("$%s", k.bits.addr.bits.lbl);
			else
				printf("%ld", k.bits.addr.bits.memaddr);
			anyptr = 1;
			break;

		case CONST_STRK:
			stringlit_use(k.bits.str->lit);
			printf("$%s", k.bits.str->lit->lbl);
			anyptr = 1;
			break;
	}

	if(k.offset)
		printf(" add %" NUMERIC_FMT_D, k.offset);
	if(anyptr)
		printf(" anyptr");
}

static void gen_ir_zeroinit(irctx *ctx, type *ty)
{
	type *test;
	if((test = type_is_primitive(ty, type_struct))){
		ICE("TODO: zeroinit: struct");
	}else if((test = type_is(ty, type_array))){
		type *elem = type_next(test);
		size_t i;
		const size_t len = type_array_len(test);

		printf("{");
		for(i = 0; i < len; i++){
			gen_ir_init_r(ctx, NULL, elem);

			if(i < len - 1)
				printf(", ");
		}
		printf("}");

	}else if((test = type_is_primitive(ty, type_union))){
		ICE("TODO: zeroinit: union");
	}else{
		printf("0");
	}
}

static void gen_ir_out_bitfields(
		irctx *ctx,
		struct bitfield_val **const bitfields,
		unsigned *const nbitfields,
		type *ty)
{
	unsigned total_width;
	integral_t v = bitfields_merge(*bitfields, *nbitfields, &total_width);

	(void)ctx;

	if(total_width > 0)
		gen_ir_integral(v, ty);

	*nbitfields = 0;
	free(*bitfields);
	*bitfields = NULL;
}

static void gen_ir_init_struct(irctx *ctx, decl_init *init, type *su_ty)
{
	struct_union_enum_st *const sue = su_ty->bits.type->sue;
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

		copy_from_init = copy_from_exp->bits.complit.decl->bits.var.init.dinit;
		assert(copy_from_init->type == decl_init_brace);

		i = copy_from_init->bits.ar.inits;
	}

	printf("{ ");

	/* iterate using members, not inits */
	for(mem = sue->members;
			mem && *mem;
			mem++)
	{
		int emit_comma = !!mem[1];
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

#define DEBUG(...)

		DEBUG("init for %ld/%s, %s",
				mem - sue->members, d_mem->spel,
				di_to_use ? di_to_use->bits.expr->f_str() : NULL);

		/* only pad if we're not on a bitfield or we're on the first bitfield */
		if(!d_mem->bits.var.field_width || !first_bf){
			DEBUG("prev padding, offset=%d, end_of_last=%d",
					d_mem->struct_offset, end_of_last);

			UCC_ASSERT(
					d_mem->bits.var.struct_offset >= end_of_last,
					"negative struct pad, sue %s, member %s "
					"offset %u, end_of_last %u",
					sue->spel, decl_to_str(d_mem),
					d_mem->bits.var.struct_offset, end_of_last);
		}

		if(d_mem->bits.var.field_width){
			if(!first_bf || d_mem->bits.var.first_bitfield){
				if(first_bf){
					DEBUG("new bitfield group (%s is new boundary), old:",
							d_mem->spel);
					/* next bitfield group - store the current */
					gen_ir_out_bitfields(ctx, &bitfields, &nbitfields, first_bf->ref);
					if(emit_comma)
						printf(", ");
				}
				first_bf = d_mem;
			}

			assert(!di_to_use || di_to_use->type == decl_init_scalar);

			bitfields = bitfields_add(
					bitfields, &nbitfields,
					d_mem, di_to_use ? di_to_use->bits.expr : NULL);

			emit_comma = 0;

		}else{
			if(nbitfields){
				DEBUG("at non-bitfield, prev-bitfield out:", 0);
				gen_ir_out_bitfields(ctx, &bitfields, &nbitfields, first_bf->ref);
				first_bf = NULL;
			}

			DEBUG("normal init for %s:", d_mem->spel);
			gen_ir_init_r(ctx, di_to_use, d_mem->ref);
		}

		if(type_is_incomplete_array(d_mem->ref)){
			UCC_ASSERT(!mem[1], "flex-arr not at end");
		}else if(!d_mem->bits.var.field_width || d_mem->bits.var.first_bitfield){
			unsigned last_sz = type_size(d_mem->ref, NULL);

			end_of_last = d_mem->bits.var.struct_offset + last_sz;
			DEBUG("done with member \"%s\", end_of_last = %d",
					d_mem->spel, end_of_last);
		}

		if(emit_comma)
			printf(", ");

#undef DEBUG
	}

	if(nbitfields)
		gen_ir_out_bitfields(ctx, &bitfields, &nbitfields, first_bf->ref);
	free(bitfields);

	printf(" }");
}

static void gen_ir_init_r(irctx *ctx, decl_init *init, type *ty)
{
	type *test;

	if(init == DYNARRAY_NULL)
		init = NULL;

	if(!init){
		if(type_is_incomplete_array(ty))
			/* flexarray */
			;
		else
			gen_ir_zeroinit(ctx, ty);
		return;
	}

	if((test = type_is_primitive(ty, type_struct))){
		gen_ir_init_struct(ctx, init, test);

	}else if((test = type_is(ty, type_array))){
		type *elem = type_next(test);
		size_t i;
		const size_t len = type_array_len(test);
		decl_init **p = init->bits.ar.inits;

		printf("{");

		assert(init->type == decl_init_brace);

		for(i = 0; i < len; i++){
			decl_init *this = NULL;

			if((this = *p)){
				p++;

				if(this != DYNARRAY_NULL && this->type == decl_init_copy){
					struct init_cpy *icpy = *this->bits.range_copy;
					/* resolve the copy */
					this = icpy->range_init;
				}
			}

			gen_ir_init_r(ctx, this, elem);

			if(i < len - 1)
				printf(", ");
		}

		printf("}");
	}else if((test = type_is_primitive(ty, type_union))){
		/* union inits are decl_init_brace with spaces up to the first union init,
		 * then NULL/end of the init-array */
		struct_union_enum_st *union_ = type_is_s_or_u(test);
		unsigned i;
		decl_init *u_init;
		decl *mem;
		type *mem_ty;

		UCC_ASSERT(init->type == decl_init_brace, "brace init expected");

		/* skip the empties until we get to one */
		for(i = 0; init->bits.ar.inits[i] == DYNARRAY_NULL; i++);

		u_init = init->bits.ar.inits[i];
		assert(u_init);

		mem = union_->members[i]->struct_member;
		mem_ty = mem->ref;

		printf("aliasinit %s ", irtype_str(mem_ty, ctx));

		/* union init, member at index `i' */
		if(mem->bits.var.field_width){
			struct bitfield_val bfv, *p = &bfv;
			unsigned n = 1;

			assert(u_init->type == decl_init_scalar);

			bitfields_val_set(&bfv, u_init->bits.expr, mem->bits.var.field_width);

			gen_ir_out_bitfields(ctx, &p, &n, mem->ref);
		}else{
			gen_ir_init_r(ctx, u_init, mem_ty);
		}

	}else{
		gen_ir_init_scalar(init);
	}
}

static void gen_ir_init(irctx *ctx, decl *d)
{
	printf(" %s ", decl_linkage(d) == linkage_internal ? "internal" : "global");

	if(attribute_present(d, attr_weak))
		printf("weak ");

	if(type_is_const(d->ref))
		printf("const ");

	if(decl_linkage(d) == linkage_external
	&& decl_init_is_zero(d->bits.var.init.dinit)
	&& d->bits.var.init.compiler_generated
	&& fopt_mode & FOPT_COMMON)
	{
		printf("common ");
	}

	gen_ir_init_r(ctx, d->bits.var.init.dinit, d->ref);
}

static void gen_ir_dump_su(struct_union_enum_st *su, irctx *ctx)
{
	size_t i;

	gen_ir_comment(ctx, "struct %s:", su->spel);

	if(!su->members)
		return;

	for(i = 0; ; i++){
		sue_member *su_mem = su->members[i];
		decl *memb;
		unsigned *idxes, nidxes, j;
		expr *fwidth;
		const char *idx_sep = "";

		if(!su_mem)
			break;

		memb = su_mem->struct_member;
		if(!irtype_struct_decl_index(su, memb, &idxes, &nidxes)){
			fprintf(stderr, "couldn't get indices for \"%s\"\n", memb->spel);
			continue;
		}

		fwidth = memb->bits.var.field_width;

		gen_ir_comment_opts(
				IR_COMMENT_CHAR,
				ctx,
				"  %s index%s ",
				memb->spel ? memb->spel : "?",
				nidxes > 1 ? "-path" : "");

		for(j = nidxes; j > 0; idx_sep = ", ", j--)
			gen_ir_comment_opts(0, ctx, "%s%u", idx_sep, idxes[j - 1]);

		gen_ir_comment_opts(
				IR_COMMENT_NEWLINE,
				ctx,
				" (field_width = %d%s)",
				fwidth ? (int)const_fold_val_i(fwidth) : -1,
				memb->bits.var.first_bitfield ? " [first]" : "");

		free(idxes);
	}
}

static void gen_ir_memset_small(irctx *ctx, irval *v, char ch, size_t len)
{
	assert(len <= 8);

	if(len == 0)
		return;

	if(is_pow2(len)){
		/* easy */
		const unsigned cast_tmp = ctx->curval++;

		printf("$%u = ptrcast i%d*, %s\n",
				cast_tmp, (int)len,
				irval_str(v, ctx));

		printf("store $%u, i%d 0x%x\n",
				cast_tmp, (int)len, ch);

	}else{
		/* just emit unrolled assignments,
		 * storing and loading (in lieu of phi:s) is just silly for < 8 bytes
		 */
		unsigned remain = len;
		irval *iter = v;
		irval iter_tmp;

		gen_ir_comment(ctx, "unrolled memset(%s, 0x%x, %u)",
				irval_str(iter, ctx), ch, (unsigned)len);

		for(;;){
			unsigned rounded = round_down_pow2(remain);
			unsigned addtmp, casttmp;

			remain -= rounded;

			gen_ir_memset_small(ctx, iter, ch, rounded);

			if(!remain)
				break;

			casttmp = ctx->curval++;
			addtmp = ctx->curval++;

			printf("$%u = ptrcast i%d, %s\n", casttmp, rounded, irval_str(iter, ctx));
			printf("$%u = ptradd $%u, i4 1\n", addtmp, casttmp);

			iter_tmp.type = IRVAL_ID;
			iter_tmp.bits.id = addtmp;
			iter = &iter_tmp;
		}
	}
}

static void gen_ir_memset_large(
		irctx *ctx, irval *const iter, char ch, size_t len, unsigned word_size)
{
	const unsigned blk_loop = ctx->curlbl++;
	const unsigned blk_fin = ctx->curlbl++;
	const unsigned byte_cnt = ctx->curval++;
	const unsigned byte_tmp = ctx->curval++;
	const unsigned byte_load = ctx->curval++;
	const unsigned byte_cmp = ctx->curval++;
	const unsigned iter_voidp = ctx->curval++;
	const unsigned iter_store = ctx->curval++;
	const unsigned iter_tmp = ctx->curval++;
	const unsigned iter_cast = ctx->curval++;
	const unsigned iter_add = ctx->curval++;
	const unsigned iter_storeback = ctx->curval++;
	irval iter_passtmp;

	assert(len > word_size);

	printf("$%u = alloca i%d\n", byte_cnt, word_size);
	printf("store $%u, i%d 0x%x\n", byte_cnt, word_size, (unsigned)len);
	printf("$%u = alloca void*\n", iter_store);
	printf("$%u = ptrcast void*, %s\n", iter_voidp, irval_str(iter, ctx));
	printf("store $%u, $%u\n", iter_store, iter_voidp);

	printf("$%u:\n", blk_loop);
	printf("$%u = load $%u\n", iter_tmp, iter_store);

	iter_passtmp.type = IRVAL_ID;
	iter_passtmp.bits.id = iter_tmp;

	gen_ir_memset_small(ctx, &iter_passtmp, ch, word_size);

	printf("$%u = load $%u\n", byte_load, byte_cnt);
	printf("$%u = sub $%u, i%d 0x%x\n", byte_tmp, byte_load, word_size, word_size);
	printf("store $%u, $%u\n", byte_cnt, byte_tmp);

	printf("$%u = ptrcast i%d*, $%u\n", iter_cast, word_size, iter_tmp);
	printf("$%u = ptradd $%u, i4 1\n", iter_add, iter_cast);
	printf("$%u = ptrcast void*, $%u\n", iter_storeback, iter_add);
	printf("store $%u, $%u\n", iter_store, iter_storeback);

	/* byte_cnt must be > 0 here, since initial len is > word_size
	 * if it's > 8, we continue, otherwise we break and copy the remaining bytes
	 */
	printf("$%u = gt $%u, i%d 0x%x\n",
			byte_cmp, byte_tmp, word_size, word_size);

	printf("br $%u, $%u, $%u\n", byte_cmp, blk_loop, blk_fin);

	printf("$%u:\n", blk_fin);

	gen_ir_memset_small(ctx, &iter_passtmp, ch, len % word_size);
}

static void gen_ir_memcpy_small(
		irctx *ctx, irval *dest_iter, irval *src_iter, size_t len)
{
	assert(len <= 8);

	if(len == 0)
		return;

	if(is_pow2(len)){
		/* easy */
		const unsigned dest_cast = ctx->curval++;
		const unsigned src_cast = ctx->curval++;
		const unsigned load = ctx->curval++;

		printf("$%u = ptrcast i%d*, %s\n",
				dest_cast, (int)len,
				irval_str(dest_iter, ctx));

		printf("$%u = ptrcast i%d*, %s\n",
				src_cast, (int)len,
				irval_str(src_iter, ctx));

		printf("$%u = load $%u\n", load, src_cast);

		printf("store $%u, $%u\n", dest_cast, load);

	}else{
		/* as with memset(), just emit unrolled assignments */
		unsigned remain = len;
		irval *iter_dest = dest_iter;
		irval *iter_src = src_iter;
		irval iter_tmp_dest, iter_tmp_src;

		gen_ir_comment(ctx, "unrolled memcpy(..., %u)", (unsigned)len);

		for(;;){
			unsigned rounded = round_down_pow2(remain);
			unsigned addtmp_dest, addtmp_src;

			remain -= rounded;

			gen_ir_memcpy_small(ctx, iter_dest, iter_src, rounded);

			if(!remain)
				break;

			addtmp_dest = ctx->curval++;
			addtmp_src = ctx->curval++;

			printf("$%u = ptradd %s, i4 1\n", addtmp_dest, irval_str(iter_dest, ctx));
			printf("$%u = ptradd %s, i4 1\n", addtmp_src, irval_str(iter_src, ctx));

			iter_tmp_dest.type = IRVAL_ID;
			iter_tmp_dest.bits.id = addtmp_dest;
			iter_dest = &iter_tmp_dest;

			iter_tmp_src.type = IRVAL_ID;
			iter_tmp_src.bits.id = addtmp_src;
			iter_src = &iter_tmp_src;
		}
	}
}

static void gen_ir_memcpy_large(
		irctx *ctx, irval *dest_iter, irval *src_iter, size_t len, unsigned word_size)
{
	const unsigned blk_loop = ctx->curlbl++;
	const unsigned blk_fin = ctx->curlbl++;
	const unsigned byte_cnt = ctx->curval++;
	const unsigned byte_tmp = ctx->curval++;
	const unsigned byte_load = ctx->curval++;
	const unsigned byte_cmp = ctx->curval++;
	const unsigned iter_dest_voidp = ctx->curval++, iter_src_voidp = ctx->curval++;
	const unsigned iter_dest_store = ctx->curval++, iter_src_store = ctx->curval++;
	const unsigned iter_dest_tmp = ctx->curval++,   iter_src_tmp = ctx->curval++;
	const unsigned iter_dest_add = ctx->curval++,   iter_src_add = ctx->curval++;
	irval iter_dest_passtmp, iter_src_passtmp;
	type *const word_size_type = type_nav_MAX_FOR(cc1_type_nav, word_size);
	const char *const word_size_type_str = irtype_str(word_size_type, ctx);

	assert(len > word_size);

	printf("$%u = alloca i%d\n", byte_cnt, word_size);
	printf("store $%u, i%d 0x%x\n", byte_cnt, word_size, (unsigned)len);

	/* alloca the pointers as T*, where T is a word_size int,
	 * since we'll be performing most assignments through
	 * that type
	 */
	printf("$%u = alloca %s*\n", iter_dest_store, word_size_type_str);
	printf("$%u = ptrcast %s*, %s\n",
			iter_dest_voidp, word_size_type_str, irval_str(dest_iter, ctx));
	printf("store $%u, $%u\n", iter_dest_store, iter_dest_voidp);

	printf("$%u = alloca %s*\n", iter_src_store, word_size_type_str);
	printf("$%u = ptrcast %s*, %s\n",
			iter_src_voidp, word_size_type_str, irval_str(src_iter, ctx));
	printf("store $%u, $%u\n", iter_src_store, iter_src_voidp);

	printf("$%u:\n", blk_loop);
	printf("$%u = load $%u\n", iter_dest_tmp, iter_dest_store);
	printf("$%u = load $%u\n", iter_src_tmp, iter_src_store);

	iter_dest_passtmp.type = IRVAL_ID;
	iter_dest_passtmp.bits.id = iter_dest_tmp;
	iter_src_passtmp.type = IRVAL_ID;
	iter_src_passtmp.bits.id = iter_src_tmp;

	gen_ir_memcpy_small(ctx, &iter_dest_passtmp, &iter_src_passtmp, word_size);

	printf("$%u = load $%u\n", byte_load, byte_cnt);
	printf("$%u = sub $%u, i%d 0x%x\n", byte_tmp, byte_load, word_size, word_size);
	printf("store $%u, $%u\n", byte_cnt, byte_tmp);

	printf("$%u = ptradd $%u, i4 1\n", iter_dest_add, iter_dest_tmp);
	printf("store $%u, $%u\n", iter_dest_store, iter_dest_add);
	printf("$%u = ptradd $%u, i4 1\n", iter_src_add, iter_src_tmp);
	printf("store $%u, $%u\n", iter_src_store, iter_src_add);

	/* byte_cnt must be > 0 here, since initial len is > word_size
	 * if it's > 8, we continue, otherwise we break and copy the remaining bytes
	 */
	printf("$%u = gt $%u, i%d 0x%x\n",
			byte_cmp, byte_tmp, word_size, word_size);

	printf("br $%u, $%u, $%u\n", byte_cmp, blk_loop, blk_fin);

	printf("$%u:\n", blk_fin);

	gen_ir_memcpy_small(ctx, &iter_dest_passtmp, &iter_src_passtmp, len % word_size);
}

void gen_ir_memset(irctx *ctx, irval *v, char ch, size_t len)
{
	unsigned ws = platform_word_size();

	if(len <= ws)
		gen_ir_memset_small(ctx, v, ch, len);
	else
		gen_ir_memset_large(ctx, v, ch, len, ws);
}

void gen_ir_memcpy(irctx *ctx, irval *dest, irval *src, size_t len)
{
	unsigned ws = platform_word_size();

	printf("# memcpy %s -> ", irval_str(src, ctx));
	printf("%s, size %lu\n", irval_str(dest, ctx), len);

	if(len <= ws)
		gen_ir_memcpy_small(ctx, dest, src, len);
	else
		gen_ir_memcpy_large(ctx, dest, src, len, ws);

	printf("# memcpy complete\n");
}

static void gen_ir_decl(decl *d, irctx *ctx)
{
	funcargs *args = type_is(d->ref, type_func) ? type_funcargs(d->ref) : NULL;

	if((d->store & STORE_MASK_STORE) == store_typedef)
		return;

	if(!d->spel){
		struct_union_enum_st *su = type_is_s_or_u(d->ref);
		if(su){
			if(IR_DUMP_FIELD_OFFSETS)
				gen_ir_dump_su(su, ctx);
		}

		return;
	}

	printf("$%s = %s", decl_asm_spel(d), irtype_str_maybe_fn(d->ref, args, ctx));

	if(args){
		putchar('\n');
		if(decl_should_emit_code(d)){
			type *retty = type_called(d->ref, NULL);

			printf("{\n");
			gen_ir_spill_args(ctx, args);
			gen_ir_stmt(d->bits.func.code, ctx);

			/* if non-void function and function may fall off the end, dummy a return */
			if(d->bits.func.control_returns_undef){
				printf("ret %s undef\n", irtype_str(retty, ctx));
			}else if(type_is_void(retty)){
				printf("ret void\n");
			}

			printf("}\n");
		}
	}else{
		if((d->store & STORE_MASK_STORE) != store_extern
		&& d->bits.var.init.dinit)
		{
			gen_ir_init(ctx, d);
		}
		putchar('\n');
	}
}

static void gen_ir_stringlit(const stringlit *lit, int predeclare)
{
	printf("$%s = [i%c x 0]", lit->lbl, lit->wide ? '4' : '1', lit->lbl);

	if(!predeclare){
		printf(" internal \"");
		literal_print(stdout, lit->str, lit->len);
		printf("\"");
	}
	printf("\n");
}

static void gen_ir_stringlits(dynmap *litmap, int predeclare)
{
	const stringlit *lit;
	size_t i;
	for(i = 0; (lit = dynmap_value(stringlit *, litmap, i)); i++)
		if(lit->use_cnt > 0 || predeclare)
			gen_ir_stringlit(lit, predeclare);
}

static void gen_ir_su_types(struct type_nav *type_nav, irctx *ctx)
{
	dynmap *suetypes = type_nav_suetypes(type_nav);
	struct_union_enum_st *su;
	size_t i;

	for(i = 0;
	    (su = dynmap_key(struct_union_enum_st *, suetypes, i));
			i++)
	{
		if(su->primitive == type_enum)
			continue;

		printf("type %s = ", irtype_su_str_abbreviated(su, ctx));
		printf("%s\n", irtype_su_str_full(su, ctx));
	}
}

static void gen_ir_local_funcs(decl **local_funcs, irctx *ctx)
{
	decl **i;
	for(i = local_funcs; i && *i; i++)
		gen_ir_decl(*i, ctx);
}

void gen_ir(symtable_global *globs)
{
	irctx ctx = { 0 };
	symtable_gasm **iasm = globs->gasms;
	decl **diter;

	gen_ir_stringlits(globs->literals, 1);
	gen_ir_su_types(cc1_type_nav, &ctx);
	gen_ir_local_funcs(globs->local_funcs, &ctx);

	for(diter = symtab_decls(&globs->stab); diter && *diter; diter++){
		decl *d = *diter;

		while(iasm && d == (*iasm)->before){
			IRTODO("emit global __asm__");

			if(!*++iasm)
				iasm = NULL;
		}

		gen_ir_decl(d, &ctx);
	}

	gen_ir_stringlits(globs->literals, 0); /* must be after code-gen - use_cnt */

	if(ctx.structints)
		dynmap_free(ctx.structints);
}

static const char *irtype_btype_str(const btype *bt)
{
	switch(bt->primitive){
		case type_void:
			return "void";

		case type__Bool:
		case type_nchar:
		case type_schar:
		case type_uchar:
			return "i1";

		case type_int:
		case type_uint:
			return "i4";

		case type_short:
		case type_ushort:
			return "i2";

		case type_long:
		case type_ulong:
			if(IS_32_BIT())
				return "i4";
			return "i8";

		case type_llong:
		case type_ullong:
			return "i8";

		case type_float:
			return "f4";
		case type_double:
			return "f8";
		case type_ldouble:
			return "flarge";

		case type_enum:
			switch(bt->sue->size){
				case 1: return "i1";
				case 2: return "i2";
				case 4: return "i4";
				case 8: return "i8";
			}
			assert(0 && "unreachable");

		case type_struct:
		case type_union:
			assert(0 && "s/u");

		case type_unknown:
			assert(0 && "unknown type");
	}
}

const char *ir_op_str(enum op_type op, int arith_rshift)
{
	switch(op){
		case op_multiply: return "mul";
		case op_divide:   return "div";
		case op_modulus:  return "mod";
		case op_plus:     return "add";
		case op_minus:    return "sub";
		case op_xor:      return "xor";
		case op_or:       return "or";
		case op_and:      return "and";
		case op_shiftl:   return "shiftl";
		case op_shiftr:
			return arith_rshift ? "shiftr_arith" : "shiftr_logic";

		case op_eq:       return "eq";
		case op_ne:       return "ne";
		case op_le:       return "le";
		case op_lt:       return "lt";
		case op_ge:       return "ge";
		case op_gt:       return "gt";

		case op_not:
			assert(0 && "operator! should use cmp with zero");
		case op_bnot:
			assert(0 && "operator~ should use xor with -1");

		case op_orsc:
		case op_andsc:
			assert(0 && "invalid op string (shortcircuit)");

		case op_unknown:
			assert(0 && "unknown op");
	}
}

static void irtype_struct_decl_add_index(
		unsigned **const out_idxs,
		unsigned *const n_out_idx,
		unsigned idx)
{
	++*n_out_idx;
	*out_idxs = urealloc1(*out_idxs, *n_out_idx * sizeof(*out_idxs));

	(*out_idxs)[*n_out_idx - 1] = idx;
}

/* precondition: expects
 *	unsigned **const out_idxs, // NULL
 *	unsigned *const n_out_idx, // 0
 */
static int irtype_struct_decl_index_type(
		struct_union_enum_st *su,
		decl *d,
		unsigned **const out_idxs,
		unsigned *const n_out_idx,
		type **const out_type)
{
	size_t i, ir_idx = 0;

	*out_type = NULL;

	for(i = 0; ; i++){
		sue_member *su_mem = su->members[i], *su_memnext;
		decl *memb, *next;
		struct_union_enum_st *sub;

		if(!su_mem)
			break;

		memb = su_mem->struct_member;

		/* anonymous sub-struct? */
		if(!memb->spel
		&& (sub = type_is_s_or_u(memb->ref))
		&& irtype_struct_decl_index_type(sub, d, out_idxs, n_out_idx, out_type))
		{
			/* found in anon sub-struct - include index in current struct */
			irtype_struct_decl_add_index(out_idxs, n_out_idx, ir_idx);
			return 1;
		}

		if(!memb->bits.var.field_width || memb->bits.var.first_bitfield)
			*out_type = memb->ref;

		if(memb == d){
			/* should've been set from .first_bitfield or first member: */
			assert(*out_type);

			irtype_struct_decl_add_index(out_idxs, n_out_idx, ir_idx);

			return 1;
		}

		su_memnext = su->members[i + 1];
		if(!su_memnext)
			break;
		next = su_memnext->struct_member;
		assert(next);

		/* if next is zero-width, we have the previous index */
		if(next->bits.var.field_width && const_fold_val_i(next->bits.var.field_width) == 0)
			continue;

		/* calculate index of next field: */
		if(memb->bits.var.field_width){
			/* we are a bitfield - is next a bitfield? */
			if(next->bits.var.field_width){
				/* if it's a bitfield boundary, increment */
				if(next->bits.var.first_bitfield)
					ir_idx++;
				/* else it's part of us */
			}else{
				/* next not a bitfield: */
				ir_idx++;
			}
		}else{
			/* current not bitfield */
			ir_idx++;
		}

	}

	return 0;
}

static void irtype_struct_decl_index_type_init(
		unsigned **const out_idxs,
		unsigned *const n_out_idx)
{
	*out_idxs = NULL;
	*n_out_idx = 0;
}

int irtype_struct_decl_index(
		struct_union_enum_st *su,
		decl *d,
		unsigned **const out_idxs,
		unsigned *const n_out_idx)
{
	type *unused;
	irtype_struct_decl_index_type_init(out_idxs, n_out_idx);
	return irtype_struct_decl_index_type(su, d, out_idxs, n_out_idx, &unused);
}

type *irtype_struct_decl_type(struct_union_enum_st *su, decl *memb)
{
	unsigned *unused, unused_cnt;
	type *ty = NULL;
	int found;

	irtype_struct_decl_index_type_init(&unused, &unused_cnt);
	found = irtype_struct_decl_index_type(su, memb, &unused, &unused_cnt, &ty);

	assert(found);
	free(unused), unused = NULL;

	return ty;
}

static unsigned irtype_struct_num(irctx *ctx, struct_union_enum_st *su)
{
	intptr_t val;

	if(!ctx->structints)
		ctx->structints = dynmap_new(struct_union_enum_st *, /*refeq*/NULL, sue_hash);

	val = dynmap_get(struct_union_enum_st *, intptr_t, ctx->structints, su);
	if(val == 0){
		/* not present */
		val = ++ctx->curtype;

		dynmap_set(struct_union_enum_st *, intptr_t, ctx->structints, su, val);
	}

	return val;
}

static const char *irtype_su_str_abbreviated(struct_union_enum_st *su, irctx *ctx)
{
	static char buf[256];

	snprintf(buf, sizeof(buf),
			"$%s%u_%s",
			su->primitive == type_struct ? "struct" : "union",
			irtype_struct_num(ctx, su),
			su->anon ? "" : su->spel);

	return buf;
}

static int uint_array_equal(
		unsigned *current_idxs, unsigned current_idx_cnt,
		unsigned *new_idxs, unsigned new_idx_cnt)
{
	unsigned i;

	if(current_idx_cnt != new_idx_cnt)
		return 0;

	for(i = 0; i < current_idx_cnt; i++)
		if(current_idxs[i] != new_idxs[i])
			return 0;

	return 1;
}

static const char *irtype_su_str_full(struct_union_enum_st *su, irctx *ctx)
{
	static char buf[256];
	strbuf_fixed sbuf = STRBUF_FIXED_INIT_ARRAY(buf);

	switch(su->primitive){
		case type_struct:
		{
			int first = 1;
			sue_member **i;
			unsigned *current_idxs = NULL, current_idx_cnt = 0;

			strbuf_fixed_printf(&sbuf, "{");

			for(i = su->members; i && *i; i++, first = 0){
				decl *memb = (*i)->struct_member;
				unsigned *new_idxs, new_idx_cnt;
				int found = irtype_struct_decl_index(su, memb, &new_idxs, &new_idx_cnt);

				assert(found);
				if(uint_array_equal(current_idxs, current_idx_cnt, new_idxs, new_idx_cnt)){
					free(new_idxs);
					continue;
				}

				/* current = new */
				free(current_idxs);
				current_idxs = new_idxs;
				current_idx_cnt = new_idx_cnt;

				if(!first)
					strbuf_fixed_printf(&sbuf, ", ");

				irtype_str_r(&sbuf, memb->ref, ctx);
			}

			free(current_idxs), current_idxs = NULL;

			strbuf_fixed_printf(&sbuf, "}");
			break;
		}

		case type_union:
		{
			sue_member **i;
			struct szalign {
				unsigned size, align;
			} largest = { 0 };
			expr *esize;
			type *unionty;
			unsigned packed_size;

			for(i = su->members; i && *i; i++){
				decl *memb = (*i)->struct_member;
				struct szalign new;

				new.size = decl_size(memb);
				new.align = decl_align(memb);

				if(new.size > largest.size)
					largest.size = new.size;
				if(new.align > largest.align)
					largest.align = new.align;
			}

			packed_size = pack_to_align(largest.size, largest.align);

			/* { [i8 x N] } */
			esize = expr_new_val(packed_size);
			FOLD_EXPR(esize, NULL);

			unionty = type_array_of(
					type_nav_btype(cc1_type_nav, type_nchar),
					esize);

			/* alignment is taken care of where the decl is allocated */

			return irtype_str_r(&sbuf, unionty, ctx);
		}

		default:
			assert(0 && "unreachable");
	}

	return strbuf_fixed_detach(&sbuf);
}

static const char *irtype_str_maybe_fn_r(
		strbuf_fixed *buf,
		type *t, irctx *ctx, funcargs *maybe_args)
{
	t = type_skip_all(t);

	switch(t->type){
		case type_btype:
			switch(t->bits.type->primitive){
				case type_struct:
				case type_union:
					strbuf_fixed_printf(buf, "%s",
							irtype_su_str_abbreviated(t->bits.type->sue, ctx));
					break;

				default:
					strbuf_fixed_printf(buf, "%s", irtype_btype_str(t->bits.type));
					break;
			}
			break;

		case type_ptr:
		case type_block:
			irtype_str_maybe_fn_r(buf, t->ref, ctx, NULL);
			strbuf_fixed_printf(buf, "*");
			break;

		case type_func:
		{
			size_t i;
			int first = 1;
			funcargs *fargs = t->bits.func.args;
			decl **arglist = fargs->arglist;
			int have_arg_names = 1;
			int variadic_or_any;

			for(i = 0; arglist && arglist[i]; i++){
				decl *d;
				if(!maybe_args){
					have_arg_names = 0;
					break;
				}
				d = maybe_args->arglist[i];
				if(!d)
					break;
				if(!d->spel){
					have_arg_names = 0;
					break;
				}
			}

			irtype_str_maybe_fn_r(buf, t->ref, ctx, NULL);
			strbuf_fixed_printf(buf, "(");

			/* ignore fargs->args_old_proto
			 * technically args_old_proto means the [ir]type is T(...)
			 * however, then we don't have access to arguments.
			 * therefore, the function must be cast to T(...) at all call
			 * sites, to enable us to call it with any arguments, as a valid C old-function
			 */

			for(i = 0; arglist && arglist[i]; i++){
				if(!first)
					strbuf_fixed_printf(buf, ", ");

				irtype_str_maybe_fn_r(buf, arglist[i]->ref, ctx, NULL);

				if(have_arg_names){
					decl *arg = maybe_args->arglist[i];

					/* use arg->spel, as decl_asm_spel() is used for the alloca-version */
					strbuf_fixed_printf(buf, " $%s", arg->spel);
				}

				first = 0;
			}

			variadic_or_any = (FUNCARGS_EMPTY_NOVOID(fargs) || fargs->variadic);

			strbuf_fixed_printf(buf, "%s%s)",
					variadic_or_any && fargs->arglist ? ", " : "",
					variadic_or_any ? "..." : "");
			break;
		}

		case type_array:
			strbuf_fixed_printf(buf, "[");
			irtype_str_maybe_fn_r(buf, t->ref, ctx, NULL);
			strbuf_fixed_printf(buf, " x %" NUMERIC_FMT_D "]",
					const_fold_val_i(t->bits.array.size));
			break;

		default:
			assert(0 && "unskipped type");
	}

	return strbuf_fixed_str(buf);
}

const char *irtype_str_r(strbuf_fixed *buf, type *t, irctx *ctx)
{
	return irtype_str_maybe_fn_r(buf, t, ctx, NULL);
}

static const char *irtype_str_maybe_fn(type *t, funcargs *args, irctx *ctx)
{
	static char buf[128];
	strbuf_fixed sbuf = STRBUF_FIXED_INIT_ARRAY(buf);

	return irtype_str_maybe_fn_r(&sbuf, t, ctx, args);
}

const char *irtype_str(type *t, irctx *ctx)
{
	static char buf[128];
	strbuf_fixed sbuf = STRBUF_FIXED_INIT_ARRAY(buf);

	return irtype_str_r(&sbuf, t, ctx);
}

const char *irval_str_r(strbuf_fixed *buf, irval *v, irctx *ctx)
{
	switch(v->type){
		case IRVAL_LITERAL:
			irtype_str_r(buf, v->bits.lit.ty, ctx);
			strbuf_fixed_printf(buf, " %" NUMERIC_FMT_D, v->bits.lit.val);
			break;

		case IRVAL_LBL:
			strbuf_fixed_printf(buf, "$%s", v->bits.lbl);
			break;

		case IRVAL_ID:
			strbuf_fixed_printf(buf, "$%u", v->bits.id);
			break;

		case IRVAL_NAMED:
			assert(v->bits.decl->sym);
			strbuf_fixed_printf(buf, "$%s", decl_asm_spel(v->bits.decl));
			break;
	}
	return strbuf_fixed_str(buf);
}

const char *irval_str(irval *v, irctx *ctx)
{
	static char buf[256];
	strbuf_fixed sbuf = STRBUF_FIXED_INIT_ARRAY(buf);

	return irval_str_r(&sbuf, v, ctx);
}

static irval *irval_new(enum irval_type t)
{
	irval *v = umalloc(sizeof *v);
	v->type = t;
	return v;
}

irval *irval_from_id(irid id)
{
	irval *v = irval_new(IRVAL_ID);
	v->bits.id = id;
	return v;
}

irval *irval_from_l(type *ty, integral_t l)
{
	irval *v = irval_new(IRVAL_LITERAL);
	v->bits.lit.val = l;
	v->bits.lit.ty = ty;
	return v;
}

irval *irval_from_sym(irctx *ctx, struct sym *sym)
{
	irval *v = irval_new(IRVAL_NAMED);
	(void)ctx;
	v->bits.decl = sym->decl;
	return v;
}

irval *irval_from_lbl(irctx *ctx, char *lbl)
{
	irval *v = irval_new(IRVAL_LBL);
	(void)ctx;
	v->bits.lbl = lbl;
	return v;
}

irval *irval_from_noop(void)
{
	return irval_from_l(NULL, 0);
}

void irval_free(irval *v)
{
	free(v);
}

void irval_free_abi(void *v)
{
	irval_free(v);
}
