STMT_DEFS(if);


void flow_fold(stmt_flow *flow, symtable **pstab);
void flow_gen(stmt_flow *flow, symtable *stab,
		const char *end_flow_labels[2]);
void flow_end(const char *[2]);
