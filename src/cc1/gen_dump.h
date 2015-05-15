#ifndef GEN_DUMP_H
#define GEN_DUMP_H

typedef struct dump dump;

void dump_stmt(stmt *t, dump *);
void dump_expr(expr *e, dump *);

void gen_dump(symtable_global *);

#endif
