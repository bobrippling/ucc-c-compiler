STMT_DEFS_NOGEN(break);
extern func_gen_stmt *gen_stmt_break;

/* used by break + continue */
void fold_stmt_break_continue(stmt *t, char *lbl);
