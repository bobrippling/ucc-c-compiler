#ifndef GEN_DUMP_H
#define GEN_DUMP_H

#include "../util/where.h"
#include "type.h"

typedef struct dump dump;

void dump_desc(
		dump *ctx,
		const char *desc, const void *uniq, const where *loc);

void dump_desc_expr(
		dump *ctx, const char *desc, const struct expr *);

void dump_desc_expr_newline(
		dump *ctx, const char *desc, const struct expr *,
		int newline);

void dump_init(dump *, struct decl_init *);

void dump_desc_stmt(dump *ctx, const char *desc, const struct stmt *);

void dump_desc_stmt_newline(
		dump *ctx, const char *desc, const struct stmt *,
		int newline);

void dump_printf(dump *, const char *, ...)
	ucc_printflike(2, 3);
void dump_printf_indent(dump *ctx, int indent, const char *fmt, ...)
	ucc_printflike(3, 4);

void dump_inc(dump *);
void dump_dec(dump *);

void dump_strliteral_indent(dump *ctx, int indent, const char *str, size_t len);
void dump_strliteral(dump *ctx, const char *str, size_t len);

void dump_stmt(struct stmt *t, dump *);
void dump_expr(struct expr *e, dump *);

void gen_dump(symtable_global *);

#endif
