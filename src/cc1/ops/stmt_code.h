STMT_DEFS(code);

struct out_dbg_lbl;

void gen_block_decls(
		symtable *,
		struct out_dbg_lbl **const,
		out_ctx *);

void gen_block_decls_dealloca(symtable *stab, out_ctx *octx);

void gen_scope_leave(symtable *s_from, symtable *s_to, out_ctx *octx);
void gen_scope_leave_parent(symtable *s_from, out_ctx *octx);

void fold_check_scope_entry(where *, const char *desc,
		symtable *const s_from, symtable *const s_to);

void fold_shadow_dup_check_block_decls(symtable *stab);

void gen_stmt_code_m1(const stmt *s, int m1, out_ctx *);
void gen_stmt_code_m1_finish(const stmt *stmt, out_ctx *octx);
