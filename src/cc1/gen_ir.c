#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <strbuf_fixed.h>

#include "../util/util.h"
#include "../util/alloc.h"

#include "sym.h"
#include "type_is.h"
#include "expr.h"
#include "stmt.h"
#include "cc1.h" /* IS_32_BIT() */
#include "sue.h"
#include "funcargs.h"
#include "str.h" /* literal_print() */

#include "gen_ir.h"
#include "gen_ir_internal.h"

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

irval *gen_ir_expr(const struct expr *expr, irctx *ctx)
{
	return expr->f_ir(expr, ctx);
}

void gen_ir_stmt(const struct stmt *stmt, irctx *ctx)
{
	stmt->f_ir(stmt, ctx);
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

static void gen_ir_decl(decl *d, irctx *ctx)
{
	funcargs *args = type_is(d->ref, type_func) ? type_funcargs(d->ref) : NULL;

	if((d->store & STORE_MASK_STORE) == store_typedef)
		return;

	printf("$%s = %s", decl_asm_spel(d), irtype_str_maybe_fn(d->ref, args));

	if(args){
		putchar('\n');
		if(d->bits.func.code){
			printf("{\n");
			gen_ir_spill_args(ctx, args);
			gen_ir_stmt(d->bits.func.code, ctx);

			/* if non-void function and function may fall off the end, dummy a return */
			if(d->bits.func.control_returns_undef){
				printf("ret undef\n");
			}

			printf("}\n");
		}
	}else{
		printf("\n");

		if(!d->bits.var.init.compiler_generated){
			ICW("TODO: ir init for %s\n", d->spel);
		}
	}
}

static void gen_ir_stringlit(const stringlit *lit, int predeclare)
{
	printf("$%s = [i%c x 0]", lit->lbl, lit->wide ? '4' : '1', lit->lbl);

	if(!predeclare){
		printf(" \"");
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
			ICW("TODO: emit global __asm__");

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

					strbuf_fixed_printf(buf, "{");

					for(i = t->bits.type->sue->members; i && *i; i++){
						if(!first)
							strbuf_fixed_printf(buf, ", ");

						irtype_str_r(buf, (*i)->struct_member->ref, NULL);

						first = 0;
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
			decl **arglist = t->bits.func.args->arglist;
			int have_arg_names = 1;

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

			strbuf_fixed_printf(buf, ")");
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

void irval_free(irval *v)
{
	free(v);
}

void irval_free_abi(void *v)
{
	irval_free(v);
}
