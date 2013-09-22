STMT_DEFS(if);


void flow_fold(stmt_flow *flow, symtable **pstab);

basic_blk *flow_gen(
		stmt_flow *flow, symtable *stab,
		basic_blk *bb) ucc_wur;
