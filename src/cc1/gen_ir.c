#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <strbuf_fixed.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

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

static const char *irtype_str_maybe_fn(type *, funcargs *maybe_args);
static void gen_ir_init_r(irctx *, decl_init *init, type *ty);

irval *gen_ir_expr(const struct expr *expr, irctx *ctx)
{
	return expr->f_ir(expr, ctx);
}

void gen_ir_stmt(const struct stmt *stmt, irctx *ctx)
{
	stmt->f_ir(stmt, ctx);
}

void gen_ir_comment(irctx *ctx, const char *fmt, ...)
{
	va_list l;

	(void)ctx;

	printf("# ");

	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);

	putchar('\n');
}

static void gen_ir_spill_args(irctx *ctx, funcargs *args)
{
	decl **i;

	(void)ctx;

	for(i = args->arglist; i && *i; i++){
		decl *d = *i;
		const char *asm_spel = decl_asm_spel(d);

		printf("$%s = alloca %s\n", asm_spel, irtype_str(d->ref));
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
		ICE("TODO: init: union");
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

	gen_ir_init_r(ctx, d->bits.var.init.dinit, d->ref);
}

static void gen_ir_dump_su(struct_union_enum_st *su, irctx *ctx)
{
	size_t i;

	if(!su->members)
		return;

	gen_ir_comment(ctx, "struct %s:", su->spel);

	for(i = 0; ; i++){
		sue_member *su_mem = su->members[i];
		decl *memb;
		unsigned idx;

		if(!su_mem)
			break;

		memb = su_mem->struct_member;
		if(!irtype_struct_decl_index(su, memb, &idx)){
			fprintf(stderr, "couldn't get index for \"%s\"\n", memb->spel);
			continue;
		}

		gen_ir_comment(ctx, "  %s index %u (first bitfield = %d, field_width = %c)",
				memb->spel ? memb->spel : "?", idx, memb->bits.var.first_bitfield,
				"NY"[!!memb->bits.var.field_width]);
	}
}

static void gen_ir_decl(decl *d, irctx *ctx)
{
	funcargs *args = type_is(d->ref, type_func) ? type_funcargs(d->ref) : NULL;

	if((d->store & STORE_MASK_STORE) == store_typedef)
		return;

	if(!d->spel){
		struct_union_enum_st *su = type_is_s_or_u(d->ref);
		if(su && IR_DUMP_FIELD_OFFSETS)
			gen_ir_dump_su(su, ctx);

		return;
	}

	printf("$%s = %s", decl_asm_spel(d), irtype_str_maybe_fn(d->ref, args));

	if(args){
		putchar('\n');
		if(decl_should_emit_code(d)){
			printf("{\n");
			gen_ir_spill_args(ctx, args);
			gen_ir_stmt(d->bits.func.code, ctx);

			/* if non-void function and function may fall off the end, dummy a return */
			if(d->bits.func.control_returns_undef){
				printf("ret undef\n");
			}else if(type_is_void(type_called(d->ref, NULL))){
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

void gen_ir(symtable_global *globs)
{
	irctx ctx = { 0 };
	symtable_gasm **iasm = globs->gasms;
	decl **diter;

	gen_ir_stringlits(globs->literals, 1);

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

int irtype_struct_decl_index(struct_union_enum_st *su, decl *d, unsigned *const out_idx)
{
	size_t i, ir_idx = 0;

	for(i = 0; ; i++){
		sue_member *su_mem = su->members[i], *su_memnext;
		decl *memb, *next;

		if(!su_mem)
			break;

		memb = su_mem->struct_member;

		if(memb == d){
			*out_idx = ir_idx;
			return 1;
		}

		su_memnext = su->members[i + 1];
		if(!su_memnext)
			break;
		next = su_memnext->struct_member;
		assert(next);

		/* calculate index of next field: */
		if(memb->bits.var.field_width){
			/* we are a bitfield - is next a bitfield? */
			if(next->bits.var.field_width){
				/* if it's a bitfield boundary, increment, except for 0-width */
				if(next->bits.var.first_bitfield
				&& const_fold_val_i(next->bits.var.field_width) > 0)
				{
					ir_idx++;
				}
				/* else it's part of us */
			}else{
				/* next not a bitfield: */
				ir_idx++;
			}
		}else{
			/* next and current not bitfields: */
			ir_idx++;
		}

	}

	return 0;
}

static void irtype_str_r(strbuf_fixed *buf, type *t, funcargs *maybe_args)
{
	t = type_skip_all(t);

	switch(t->type){
		case type_btype:
			switch(t->bits.type->primitive){
				case type_struct:
				{
					int first = 1;
					sue_member **i;
					unsigned current_idx = -1;
					struct_union_enum_st *su = t->bits.type->sue;

					strbuf_fixed_printf(buf, "{");

					for(i = su->members; i && *i; i++, first = 0){
						decl *memb = (*i)->struct_member;
						unsigned new_idx;
						int found = irtype_struct_decl_index(su, memb, &new_idx);

						assert(found);
						if(new_idx == current_idx)
							continue;
						current_idx = new_idx;

						if(!first)
							strbuf_fixed_printf(buf, ", ");

						irtype_str_r(buf, memb->ref, NULL);
					}

					strbuf_fixed_printf(buf, "}");
					break;
				}

				case type_union:
					ICE("TODO: union type");

				default:
					strbuf_fixed_printf(buf, "%s", irtype_btype_str(t->bits.type));
					break;
			}
			break;

		case type_ptr:
		case type_block:
			irtype_str_r(buf, t->ref, NULL);
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

			irtype_str_r(buf, t->ref, NULL);
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

				irtype_str_r(buf, arglist[i]->ref, NULL);

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
			irtype_str_r(buf, t->ref, NULL);
			strbuf_fixed_printf(buf, " x %" NUMERIC_FMT_D "]",
					const_fold_val_i(t->bits.array.size));
			break;

		default:
			assert(0 && "unskipped type");
	}
}

static const char *irtype_str_maybe_fn(type *t, funcargs *maybe_args)
{
	static char ar[128];
	strbuf_fixed buf = STRBUF_FIXED_INIT_ARRAY(ar);

	irtype_str_r(&buf, t, maybe_args);

	return strbuf_fixed_detach(&buf);
}

const char *irtype_str(type *t)
{
	return irtype_str_maybe_fn(t, NULL);
}

const char *irval_str(irval *v)
{
	static char buf[256];

	switch(v->type){
		case IRVAL_LITERAL:
		{
			strbuf_fixed sbuf = STRBUF_FIXED_INIT_ARRAY(buf);
			size_t len;

			irtype_str_r(&sbuf, v->bits.lit.ty, NULL);
			len = strlen(buf);

			assert(len < sizeof buf);

			snprintf(buf + len, sizeof buf - len,
					" %" NUMERIC_FMT_D,
					v->bits.lit.val);
			break;
		}

		case IRVAL_LBL:
			snprintf(buf, sizeof buf, "$%s", v->bits.lbl);
			break;

		case IRVAL_ID:
			snprintf(buf, sizeof buf, "$%u", v->bits.id);
			break;

		case IRVAL_NAMED:
		{
			assert(v->bits.decl->sym);
			snprintf(buf, sizeof buf, "$%s", decl_asm_spel(v->bits.decl));
			break;
		}
	}
	return buf;
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
