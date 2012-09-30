func_fold_stmt fold_stmt_code;
func_gen_stmt  gen_stmt_code;
func_str_stmt  str_stmt_code;

void gen_code_decls(symtable *stab);
func_mutate_stmt mutate_stmt_code;

#define FOR_INIT_AND_CODE(i, s, dcodes, code) \
	dcodes = 0,                        \
	i = s->inits;                      \
restart:                             \
	for(; i && *i; i++){               \
		code;                            \
	}                                  \
	if(!dcodes){                       \
		dcodes = 1,                      \
		i = s->codes;                    \
		goto restart;                    \
	}

