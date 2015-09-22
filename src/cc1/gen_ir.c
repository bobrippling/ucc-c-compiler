#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"

#include "sym.h"
#include "type_is.h"
#include "expr.h"
#include "stmt.h"

#include "gen_ir.h"
#include "gen_ir_internal.h"

irval *gen_ir_expr(irctx *ctx, const struct expr *expr)
{
	return expr->f_ir(expr, ctx);
}

void gen_ir_stmt(irctx *ctx, const struct stmt *stmt)
{
	stmt->f_ir(stmt, ctx);
}

static void gen_ir_decl(decl *d, irctx *ctx)
{
	if(type_is(d->ref, type_func)){
		gen_ir_stmt(ctx, d->bits.func.code);
	}else{
		printf("$%s = %s\n", decl_asm_spel(d), irtype_str(d->ref));

		if(d->bits.var.init.dinit){
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
