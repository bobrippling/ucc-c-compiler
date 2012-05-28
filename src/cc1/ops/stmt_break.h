func_fold_stmt  fold_stmt_break;
func_gen_stmt  *gen_stmt_break;
func_str_stmt   str_stmt_break;

/* used by break + continue */
void fold_stmt_break_continue(stmt *t, char *lbl);
