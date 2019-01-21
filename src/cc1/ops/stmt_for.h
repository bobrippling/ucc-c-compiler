STMT_DEFS(for);

void dump_flow(stmt_flow *flow, struct dump *ctx);

int fold_code_escapable(stmt *s);
void stmt_for_got_decls(stmt *s);
