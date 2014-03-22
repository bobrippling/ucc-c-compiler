STMT_DEFS(switch);

/* pass lbl=NULL, &lbl for a default case */
ucc_nonnull()
void fold_stmt_and_add_to_curswitch(stmt *cse, out_blk **lbl);
