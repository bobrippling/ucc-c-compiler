#include <assert.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_struct.h"
#include "../sue.h"
#include "../out/asm.h"
#include "../type_is.h"
#include "../type_nav.h"

const char *str_expr_struct()
{
	return "member-access";
}

static type *struct_member_type(struct_union_enum_st *su, decl *d)
{
	if(!d->bits.var.field_width)
		return d->ref; /* simple */

	/*
	 * struct A { short s : 2; int i : 2; };
	 * A::i has type short, since it's in a short bitfield
	 */
	return su_member_type(su, d);
}

void fold_expr_struct(expr *e, symtable *stab)
{
	/*
	 * lhs = any ptr-to-struct expr
	 * rhs = struct member ident
	 */
	const int ptr_expect = !e->expr_is_st_dot;
	struct_union_enum_st *sue;
	char *spel;
	enum type_qualifier struct_qual;

	if(ptr_expect)
		FOLD_EXPR(e->lhs, stab);
	else
		fold_expr_nodecay(e->lhs, stab);

	/* don't fold the rhs - just a member name */
	if(e->rhs){
		UCC_ASSERT(expr_kind(e->rhs, identifier),
				"struct/union member not identifier (%s)", e->rhs->f_str());

		UCC_ASSERT(!e->bits.struct_mem.d, "already have a struct-member");

		spel = e->rhs->bits.ident.bits.ident.spel;
	}else{
		UCC_ASSERT(e->bits.struct_mem.d, "no member specified already?");
		spel = NULL;
	}

	/* we access a struct, of the right ptr depth */
	if(!(sue = expr_struct_sutype(e))){
		die_at(&e->lhs->where, "'%s' (%s-expr) is not a %sstruct or union (member %s)",
				type_to_str(e->lhs->tree_type),
				e->lhs->f_str(),
				ptr_expect ? "pointer to " : "",
				spel);
	}

	if(!sue_complete(sue)){
		char wbuf[WHERE_BUF_SIZ];

		die_at(&e->lhs->where, "%s incomplete type (%s)\n"
				"%s: note: forward declared here",
				ptr_expect
					? "dereferencing pointer to"
					: "accessing member of",
				type_to_str(e->lhs->tree_type),
				where_str_r(wbuf, &sue->where));
	}

	if(spel){
		/* found the struct, find the member */
		decl *d_mem = struct_union_member_find(sue, spel, NULL);

		if(!d_mem)
			die_at(&e->where, "%s %s has no member named \"%s\"",
					sue_str(sue), sue->spel, spel);

		e->bits.struct_mem.d = d_mem;
		e->rhs->tree_type = d_mem->ref;
	}
	assert(e->bits.struct_mem.d);

	/* pointer to struct - skip the pointer type and pull the quals
	 * off the struct type */
	struct_qual = type_qual(
			ptr_expect
			? type_dereference_decay(e->lhs->tree_type)
			: e->lhs->tree_type);

	/* pull qualifiers from the struct to the member */
	e->tree_type = type_qualify(
			struct_member_type(sue, e->bits.struct_mem.d),
			struct_qual);
}

struct_union_enum_st *expr_struct_sutype(const expr *e)
{
	type *ty = e->lhs->tree_type, *test;

	if(e->expr_is_st_dot)
		return type_is_s_or_u(ty);

	test = type_is_ptr(ty);
	if(!test){
		test = type_is(ty, type_array);
		if(!test)
			return NULL;
		test = test->ref;
	}

	return type_is_s_or_u(test);
}

static unsigned expr_struct_offset_r(
		struct_union_enum_st *su,
		decl *const target,
		unsigned *const off)
{
	sue_member **i;

	assert(su);

	for(i = su->members; i && *i; i++){
		decl *d = (*i)->struct_member;
		struct_union_enum_st *sub;

		if(d == target){
			*off = d->bits.var.struct_offset;
			return 1;
		}

		if(!d->spel && (sub = type_is_s_or_u(d->ref))
		&& expr_struct_offset_r(sub, target, off))
		{
			/* found in a substruct, add on this offset */
			*off += d->bits.var.struct_offset;
			return 1;
		}
	}

	return 0;
}

static unsigned expr_struct_offset(const expr *e)
{
	struct_union_enum_st *su = expr_struct_sutype(e);
	unsigned off = 0;
	int found;

	assert(su);
	found = expr_struct_offset_r(su, e->bits.struct_mem.d, &off);

	assert(found);
	return off;
}

const out_val *gen_expr_struct(const expr *e, out_ctx *octx)
{
	const out_val *struct_exp, *off;
	unsigned ioff;

	/* cast for void* arithmetic */

	struct_exp = out_change_type(
			octx,
			gen_expr(e->lhs, octx),
			type_ptr_to(type_nav_btype(cc1_type_nav, type_void)));

	/* find total struct offset */
	ioff = expr_struct_offset(e);

	off =
		out_op(
				octx, op_plus,
				struct_exp,
				out_new_l(
					octx,
					type_nav_btype(cc1_type_nav, type_intptr_t),
					ioff));

	if(fopt_mode & FOPT_VERBOSE_ASM)
		out_comment(octx, "struct member %s", e->bits.struct_mem.d->spel);


	{
		decl *d = e->bits.struct_mem.d;

		off = out_change_type(octx, off, type_ptr_to(d->ref));

		/* set if we're a bitfield - out_deref() and out_store()
		 * i.e. read + write then handle this
		 */
		if(d->bits.var.field_width){
			unsigned w = const_fold_val_i(d->bits.var.field_width);

			off = out_set_bitfield(
					octx, off, d->bits.var.struct_offset_bitfield, w);

			out_comment(octx, "struct bitfield lea");
		}
	}

	return off;
}

static irid gen_ir_expr_struct_elem_r(
		struct_union_enum_st *su,
		decl *const target,
		irval *struct_val,
		irctx *ctx)
{
	unsigned *indexes = 0, index_count, i;
	irid id;
	int found;

	found = irtype_struct_decl_index(su, target, &indexes, &index_count);
	assert(found);

	if(index_count > 1)
		printf("# ir indexing, index_count=%u, indexes:\n", index_count);

	for(i = index_count; i > 0; i--){
		const irid previd = id;
		unsigned memb_idx = indexes[i - 1];

		id = ctx->curval++;

		if(i == index_count)
			printf("$%u = elem %s, i4 %u\n", id, irval_str(struct_val, ctx), memb_idx);
		else
			printf("$%u = elem $%u, i4 %u\n", id, previd, memb_idx);
	}
	free(indexes);

	return id;
}

irval *gen_ir_expr_struct(const expr *e, irctx *ctx)
{
	const int ptr_expect = !e->expr_is_st_dot;
	decl *const d = e->bits.struct_mem.d;
	irval *struct_exp;
	unsigned retid;
	struct_union_enum_st *su = type_is_s_or_u(
			ptr_expect
			? type_is_ptr(e->lhs->tree_type)
			: e->lhs->tree_type);

	assert(su && "no struct type");

	struct_exp = gen_ir_expr(e->lhs, ctx);

	if(su->primitive == type_union){
		retid = ctx->curval++;

		printf("$%u = ptrcast %s, %s\n",
				retid,
				irtype_str(type_ptr_to(d->ref), ctx),
				irval_str(struct_exp, ctx));
	}else{
		retid = gen_ir_expr_struct_elem_r(su, e->bits.struct_mem.d, struct_exp, ctx);
	}

	if(d->bits.var.field_width){
		/* bitfield loading handled by expr_cast (lval2rval),
		 * special cased, since we can't deref here as we're an lvalue */
	}

	return irval_from_id(retid);
}

void dump_expr_struct(const expr *e, dump *ctx)
{
	decl *mem = e->bits.struct_mem.d;

	dump_desc_expr_newline(ctx, "member-access", e, 0);

	dump_printf_indent(ctx, 0, " %s%s\n",
			e->expr_is_st_dot ? "." : "->",
			mem->spel);

	dump_inc(ctx);
	dump_expr(e->lhs, ctx);
	dump_dec(ctx);
}

static void fold_const_expr_struct(expr *e, consty *k)
{
	/* if lhs is NULL (or some pointer constant),
	 * const fold to struct offset, (obv. if !dot, which is taken care of in fold) */
	const_fold(e->lhs, k);

	switch(k->type){
		case CONST_NO:
		case CONST_STRK:
			k->type = CONST_NO;
			break;

		case CONST_NEED_ADDR:
		case CONST_ADDR:
			/* a.b == (&a)->b, i.e. a.b wants a CONST_NEED_ADDR
			 * a->b wants a CONST_ADDR
			 */
			if(k->type == (e->expr_is_st_dot ? CONST_NEED_ADDR : CONST_ADDR)){
				/* not constant unless addressed e.g. &a->b (unless array/func) */
				k->type = CONST_ADDR_OR_NEED(e->bits.struct_mem.d);
				/* don't touch k->bits.addr info */

				/* obviously we offset this */
				k->offset += expr_struct_offset(e);
			}else{
				k->type = CONST_NO;
			}
			break;

		case CONST_NUM:
			k->type = CONST_NEED_ADDR; /* e.g. &((A *)0)->b */

			/* convert the val to a memaddr */
			/* read num.val before we clobber it */
			k->bits.addr.bits.memaddr = k->bits.num.val.i + expr_struct_offset(e);
			k->offset = 0;

			k->bits.addr.is_lbl = 0;
			break;
	}
}

static enum lvalue_kind struct_is_lval(expr *e)
{
	if(e->expr_is_st_dot){
		/* we're only an lvalue if our subexpression is a
		 * non-internal/C-standard lvalue.
		 *
		 * unless we're being checked internally, in which
		 * case we want lval2rval decay - hence yes
		 */
		return expr_is_lval(e->lhs);
	}else{
		return LVALUE_USER_ASSIGNABLE;
	}
}

void mutate_expr_struct(expr *e)
{
	e->f_const_fold = fold_const_expr_struct;
	e->f_islval = struct_is_lval;

	/* zero out the union/rhs if we're mutating */
	e->bits.struct_mem.d = NULL;
	e->rhs = NULL;
}

expr *expr_new_struct(expr *sub, int dot, expr *ident)
{
	expr *e = expr_new_wrapper(struct);
	e->expr_is_st_dot = dot;
	e->lhs = sub;
	e->rhs = ident;
	return e;
}

expr *expr_new_struct_mem(expr *sub, int dot, decl *d)
{
	expr *e = expr_new_wrapper(struct);
	e->expr_is_st_dot = dot;
	e->lhs = sub;
	e->bits.struct_mem.d = d;
	return e;
}

const out_val *gen_expr_style_struct(const expr *e, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
	stylef("->%s", e->bits.struct_mem.d->spel);
	return NULL;
}
