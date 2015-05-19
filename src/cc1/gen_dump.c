#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/dynarray.h"

#include "sym.h"
#include "str.h"
#include "expr.h"
#include "decl_init.h"
#include "stmt.h"
#include "type_is.h"
#include "sue.h"
#include "funcargs.h"

#include "gen_dump.h"

struct dump
{
	FILE *fout;
	unsigned indent;
};

static void dump_decl(decl *d, dump *ctx, const char *desc);

static void dump_indent(dump *ctx)
{
	unsigned i;
	for(i = ctx->indent; i; i--)
		fputc(' ', ctx->fout);
}

static void dump_newline(dump *ctx, int newline)
{
	if(newline)
		fputc('\n', ctx->fout);
}

static void dump_desc_newline(
		dump *ctx,
		const char *desc, const void *uniq, const where *loc,
		int newline)
{
	dump_indent(ctx);

	fprintf(ctx->fout, "%s %p <%s>", desc, uniq, where_str(loc));

	dump_newline(ctx, newline);
}

void dump_desc(
		dump *ctx,
		const char *desc, const void *uniq, const where *loc)
{
	dump_desc_newline(ctx, desc, uniq, loc, 1);
}

void dump_desc_expr_newline(
		dump *ctx, const char *desc, const struct expr *e,
		int newline)
{
	dump_desc_newline(ctx, desc, e, &e->where, 0);

	if(e->tree_type)
		fprintf(ctx->fout, " '%s'", type_to_str(e->tree_type));

	dump_newline(ctx, newline);
}

void dump_desc_stmt(dump *ctx, const char *desc, const struct stmt *s)
{
	dump_desc(ctx, desc, s, &s->where);
}

void dump_desc_expr(dump *ctx, const char *desc, const expr *e)
{
	dump_desc_expr_newline(ctx, desc, e, 1);
}

void dump_strliteral(dump *ctx, const char *str, size_t len)
{
	fprintf(ctx->fout, "\"");
	literal_print(ctx->fout, str, len);
	fprintf(ctx->fout, "\"\n");
}

void dump_expr(expr *e, dump *ctx)
{
	e->f_dump(e, ctx);
}

void dump_stmt(stmt *s, dump *ctx)
{
	s->f_dump(s, ctx);
}

void dump_init(dump *ctx, decl_init *dinit)
{
	if(dinit == DYNARRAY_NULL){
		dump_printf(ctx, "<null init>\n");
		return;
	}

	switch(dinit->type){
		case decl_init_scalar:
		{
			dump_expr(dinit->bits.expr, ctx);
			break;
		}

		case decl_init_brace:
		{
			decl_init **i;

			dump_desc(ctx, "brace init", dinit, &dinit->where);

			dump_inc(ctx);

			for(i = dinit->bits.ar.inits; i && *i; i++)
				dump_init(ctx, *i);

			dump_dec(ctx);
			break;
		}

		case decl_init_copy:
		{
			struct init_cpy *cpy = *dinit->bits.range_copy;
			dump_init(ctx, cpy->range_init);
			break;
		}
	}
}

void dump_inc(dump *ctx)
{
	ctx->indent++;
}

void dump_dec(dump *ctx)
{
	ctx->indent--;
}

static void dump_vprintf_indent(
		dump *ctx, int indent, const char *fmt, va_list l)
{
	if(indent)
		dump_indent(ctx);

	vfprintf(ctx->fout, fmt, l);
}

void dump_printf_indent(dump *ctx, int indent, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	dump_vprintf_indent(ctx, indent, fmt, l);
	va_end(l);
}

void dump_printf(dump *ctx, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	dump_vprintf_indent(ctx, 1, fmt, l);
	va_end(l);
}

static void dump_gasm(symtable_gasm *gasm, dump *ctx)
{
	dump_desc(ctx, "global asm", gasm, &gasm->where);

	dump_inc(ctx);
	dump_strliteral(ctx, gasm->asm_str, strlen(gasm->asm_str));
	dump_dec(ctx);
}

static void dump_attributes(attribute *da, dump *ctx)
{
	for(; da; da = da->next){
		dump_desc_newline(ctx, "attribute", da, &da->where, 0);

		dump_printf_indent(ctx, 0, " %s\n", attribute_to_str(da));
	}
}

static void dump_sue(dump *ctx, type *ty)
{
	struct_union_enum_st *sue = type_is_s_or_u_or_e(ty);
	sue_member **mi;

	if(!sue)
		return;

	dump_inc(ctx);

	for(mi = sue->members; mi && *mi; mi++){
		if(sue->primitive == type_enum){
			enum_member *emem = (*mi)->enum_member;

			dump_desc(ctx, emem->spel, emem, &emem->where);

		}else{
			decl *d = (*mi)->struct_member;

			dump_decl(d, ctx, "member");

			dump_sue(ctx, d->ref);
		}
	}

	dump_dec(ctx);
}

static void dump_args(funcargs *fa, dump *ctx)
{
	decl **di;

	for(di = fa->arglist; di && *di; di++)
		dump_decl(*di, ctx, "argument");
}

static void dump_decl(decl *d, dump *ctx, const char *desc)
{
	const int is_func = !!type_is(d->ref, type_func);
	type *ty;

	if(!desc){
		if(d->spel){
			desc = is_func ? "function" : "variable";
		}else{
			desc = "type";
		}
	}

	dump_desc_newline(ctx, desc, d, &d->where, 0);

	if(d->proto)
		dump_printf_indent(ctx, 0, " prev %p", (void *)d->proto);

	if(d->spel)
		dump_printf_indent(ctx, 0, " %s", d->spel);

	dump_printf_indent(ctx, 0, " '%s'", type_to_str(d->ref));

	if(d->store)
		dump_printf_indent(ctx, 0, " %s", decl_store_to_str(d->store));

	dump_printf_indent(ctx, 0, "\n");

	if(!is_func){
		if(d->bits.var.field_width){
			dump_inc(ctx);
			dump_expr(d->bits.var.field_width, ctx);
			dump_dec(ctx);
		}

		if(!d->spel){
			dump_sue(ctx, d->ref);
		}else if(d->bits.var.init.dinit){
			dump_inc(ctx);
			dump_init(ctx, d->bits.var.init.dinit);
			dump_dec(ctx);
		}
	}

	dump_inc(ctx);
	dump_attributes(d->attr, ctx);
	ty = type_skip_non_attr(d->ref);
	if(ty && ty->type == type_attr)
		dump_attributes(ty->bits.attr, ctx);
	dump_dec(ctx);

	if(is_func && d->bits.func.code){
		funcargs *fa = type_funcargs(d->ref);

		dump_inc(ctx);
		dump_args(fa, ctx);
		dump_stmt(d->bits.func.code, ctx);
		dump_dec(ctx);
	}
}

void gen_dump(symtable_global *globs)
{
	dump dump = { 0 };
	symtable_gasm **iasm = globs->gasms;
	decl **diter;

	dump.fout = stdout;

	for(diter = symtab_decls(&globs->stab); diter && *diter; diter++){
		decl *d = *diter;

		while(iasm && d == (*iasm)->before){
			dump_gasm(*iasm, &dump);

			if(!*++iasm)
				iasm = NULL;
		}

		dump_decl(d, &dump, NULL);
	}
}
