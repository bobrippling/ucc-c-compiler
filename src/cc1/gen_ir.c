#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

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

#include "gen_ir.h"
#include "gen_ir_internal.h"

struct irval
{
	enum irval_type
	{
		IRVAL_LITERAL,
		IRVAL_ID
	} type;

	union
	{
		integral_t lit;
		irid id;
	} bits;
};

irval *gen_ir_expr(const struct expr *expr, irctx *ctx)
{
	return expr->f_ir(expr, ctx);
}

void gen_ir_stmt(const struct stmt *stmt, irctx *ctx)
{
	stmt->f_ir(stmt, ctx);
}

static void gen_ir_decl(decl *d, irctx *ctx)
{
	printf("$%s = %s", decl_asm_spel(d), irtype_str(d->ref));

	if(type_is(d->ref, type_func)){
		printf("\n{\n");
		gen_ir_stmt(d->bits.func.code, ctx);
		printf("}\n");
	}else{
		printf("\n");

		if(!d->bits.var.init.compiler_generated){
			ICW("TODO: ir init for %s\n", d->spel);
		}
	}
}

void gen_ir(symtable_global *globs)
{
	irctx ctx = { 0 };
	symtable_gasm **iasm = globs->gasms;
	decl **diter;

	for(diter = symtab_decls(&globs->stab); diter && *diter; diter++){
		decl *d = *diter;

		while(iasm && d == (*iasm)->before){
			ICW("TODO: emit global __asm__");

			if(!*++iasm)
				iasm = NULL;
		}

		gen_ir_decl(d, &ctx);
	}
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

static void irtype_str_r(strbuf_fixed *buf, type *t)
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

						irtype_str_r(buf, (*i)->struct_member->ref);

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
			irtype_str_r(buf, t->ref);
			strbuf_fixed_printf(buf, "*");
			break;

		case type_func:
		{
			decl **i;
			int first = 1;

			irtype_str_r(buf, t->ref);
			strbuf_fixed_printf(buf, "(");

			for(i = t->bits.func.args->arglist; i && *i; i++){
				if(!first)
					strbuf_fixed_printf(buf, ", ");

				irtype_str_r(buf, (*i)->ref);

				first = 0;
			}

			strbuf_fixed_printf(buf, ")");
			break;
		}

		case type_array:
			strbuf_fixed_printf(buf, "[");
			irtype_str_r(buf, t->ref);
			strbuf_fixed_printf(buf, " x %" NUMERIC_FMT_D "]",
					const_fold_val_i(t->bits.array.size));
			break;

		default:
			assert(0 && "unskipped type");
	}
}

const char *irtype_str(type *t)
{
	static char ar[128];
	strbuf_fixed buf = STRBUF_FIXED_INIT_ARRAY(ar);

	irtype_str_r(&buf, t);

	return strbuf_fixed_detach(&buf);
}

const char *irval_str(irval *v)
{
	static char buf[256];

	switch(v->type){
		case IRVAL_LITERAL:
			snprintf(buf, sizeof buf, "i4_todo %" NUMERIC_FMT_D, v->bits.lit);
			break;
		case IRVAL_ID:
			snprintf(buf, sizeof buf, "$%u", v->bits.id);
			break;
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

irval *irval_from_l(integral_t l)
{
	irval *v = irval_new(IRVAL_LITERAL);
	v->bits.lit = l;
	return v;
}

irval *irval_from_sym(irctx *ctx, struct sym *sym)
{
	(void)ctx;
	(void)sym;
	assert(0 && "TODO: sym");
}

irval *irval_from_lbl(irctx *ctx, char *lbl)
{
	(void)ctx;
	(void)lbl;
	assert(0 && "TODO: label");
}

void irval_free(irval *v)
{
	free(v);
}

void irval_free_abi(void *v)
{
	irval_free(v);
}
