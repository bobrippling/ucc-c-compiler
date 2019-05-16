STMT_DEFS(label);

void gen_stmt_label_m1(const stmt *, out_ctx *, int );

int label_passable(stmt *s);

#define stmt_label_sub(l) ((l)->lhs)
