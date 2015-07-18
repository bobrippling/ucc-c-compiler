#define _POSIX_SOURCE /* fileno() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"

#include "sym.h"
#include "str.h"
#include "expr.h"
#include "decl_init.h"
#include "stmt.h"
#include "type_is.h"
#include "sue.h"
#include "funcargs.h"

#include "gen_dump.h"

static const char *const col_desc = "\x1b[1;34m";
static const char *const col_desc_decl = "\x1b[1;34m";
static const char *const col_desc_stmt = "\x1b[1;34m";
static const char *const col_desc_expr = "\x1b[1;35m";
static const char *const col_ptr = "\x1b[0;33m";
static const char *const col_where = "\x1b[0;33m";
static const char *const col_type = "\x1b[0;32m";
static const char *const col_strlit = "\x1b[1;36m";
static const char *const col_off = "\x1b[m";

struct dump
{
	FILE *fout;
	const char *last_fname;
	unsigned last_line;
	unsigned indent;
};

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

static const char *maybe_colour(FILE *f, const char *col)
{
	return isatty(fileno(f)) ? col : "";
}

static void dump_desc_colour_newline(
		dump *ctx,
		const char *desc, const void *uniq, const where *loc,
		const char *col, int newline)
{
	char *where_str;
	size_t where_str_len = 0;
	const char *fname = NULL;
	unsigned line = 0;
	const int num_len = 32;

	dump_indent(ctx);

	if(!ctx->last_fname || ctx->last_fname != loc->fname)
		fname = loc->fname;
	if(!ctx->last_line || ctx->last_line != loc->line)
		line = loc->line;

	where_str_len = (fname ? strlen(fname) : 0) + (line ? num_len : 0) + num_len;
	where_str = umalloc(where_str_len);

	if(fname)
		snprintf(where_str, where_str_len, "%s:%d:%d", fname, line, loc->chr);
	else if(line)
		snprintf(where_str, where_str_len, "line %d, col %d", line, loc->chr);
	else
		snprintf(where_str, where_str_len, "col %d", loc->chr);

	fprintf(ctx->fout, "%s%s %s%p %s<%s>%s",
			col, desc,
			maybe_colour(ctx->fout, col_ptr), uniq,
			maybe_colour(ctx->fout, col_where), where_str,
			maybe_colour(ctx->fout, col_off));

	dump_newline(ctx, newline);

	free(where_str);

	if(fname)
		ctx->last_fname = fname;
	if(line)
		ctx->last_line = line;
}

static void dump_type(dump *ctx, type *ty)
{
	dump_printf_indent(ctx, 0, " %s'%s'%s",
			maybe_colour(ctx->fout, col_type),
			type_to_str(ty),
			maybe_colour(ctx->fout, col_off));
}

void dump_desc(
		dump *ctx,
		const char *desc, const void *uniq, const where *loc)
{
	dump_desc_colour_newline(ctx, desc, uniq, loc,
			maybe_colour(ctx->fout, col_desc_decl), 1);
}

void dump_desc_expr_newline(
		dump *ctx, const char *desc, const struct expr *e,
		int newline)
{
	dump_desc_colour_newline(ctx, desc, e, &e->where,
			maybe_colour(ctx->fout, col_desc_expr), 0);

	if(e->tree_type)
		dump_type(ctx, e->tree_type);

	dump_newline(ctx, newline);
}

void dump_desc_stmt_newline(
		dump *ctx, const char *desc, const struct stmt *s,
		int newline)
{
	dump_desc_colour_newline(ctx, desc, s, &s->where,
			maybe_colour(ctx->fout, col_desc_stmt), newline);
}

void dump_desc_stmt(dump *ctx, const char *desc, const struct stmt *s)
{
	dump_desc_stmt_newline(ctx, desc, s, 1);
}

void dump_desc_expr(dump *ctx, const char *desc, const expr *e)
{
	dump_desc_expr_newline(ctx, desc, e, 1);
}

void dump_strliteral_indent(dump *ctx, int indent, const char *str, size_t len)
{
	if(indent)
		dump_indent(ctx);
	fprintf(ctx->fout, "%s\"", maybe_colour(ctx->fout, col_strlit));
	literal_print(ctx->fout, str, len);
	fprintf(ctx->fout, "\"%s\n", maybe_colour(ctx->fout, col_off));
}

void dump_strliteral(dump *ctx, const char *str, size_t len)
{
	dump_strliteral_indent(ctx, 1, str, len);
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
		dump_desc_colour_newline(ctx, "attribute",
				da, &da->where,
				maybe_colour(ctx->fout, col_desc), 0);

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

void dump_decl(decl *d, dump *ctx, const char *desc)
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

	dump_desc_colour_newline(ctx, desc, d, &d->where,
			maybe_colour(ctx->fout, col_desc_decl), 0);

	if(d->proto)
		dump_printf_indent(ctx, 0, " prev %p", (void *)d->proto);

	if(d->spel)
		dump_printf_indent(ctx, 0, " %s", d->spel);

	dump_type(ctx, d->ref);

	if(d->store)
		dump_printf_indent(ctx, 0, " %s", decl_store_to_str(d->store));

	dump_printf_indent(ctx, 0, "\n");

	if(!is_func){
		type *tof = type_skip_non_tdefs(d->ref);
		if(tof->type == type_tdef && !tof->bits.tdef.decl){
			/* show typeof expr */
			dump_inc(ctx);
			dump_expr(tof->bits.tdef.type_of, ctx);
			dump_dec(ctx);
		}

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
