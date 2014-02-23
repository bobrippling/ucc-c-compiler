#ifndef PARSE_STMT_H
#define PARSE_STMT_H

#include "../util/compiler.h"
#include "stmt.h"

struct stmt_ctx;

stmt *parse_stmt_block(symtable *, const struct stmt_ctx *const ctx);
stmt *parse_stmt(const struct stmt_ctx *ctx) ucc_nonnull();

void parse_static_assert(symtable *scope);

symtable_gasm *parse_gasm(void);

#endif
