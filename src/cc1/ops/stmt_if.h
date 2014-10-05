STMT_DEFS(if);


void flow_fold(stmt_flow *flow, symtable **pstab);

void flow_gen(stmt_flow *flow, symtable *stab,
		const char *endlbls[2], out_ctx *octx);

void flow_end(
		stmt_flow *flow, symtable *stab,
		const char *endlbls[2], out_ctx *octx);
