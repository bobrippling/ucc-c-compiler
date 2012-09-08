#include <string.h>

#include "ops.h"
#include "stmt_asm.h"
#include "../out/__asm.h"

const char *str_stmt_asm()
{
	return "asm";
}

static void check_constraint(asm_inout *io, symtable *stab, int output)
{
	fold_expr(io->exp, stab);

	out_constraint_check(&io->exp->where, io->constraints, output);
}

void fold_stmt_asm(stmt *s)
{
	asm_inout **it;
	int n_inouts;

	n_inouts = 0;

	for(it = s->asm_bits->inputs; it && *it; it++, n_inouts++)
		check_constraint(*it, s->symtab, 0);

	for(it = s->asm_bits->outputs; it && *it; it++, n_inouts++){
		asm_inout *io = *it;
		check_constraint(io, s->symtab, 1);
		if(!expr_is_lvalue(io->exp, 0))
			DIE_AT(&io->exp->where, "asm output not an lvalue");
	}

	/* validate asm string - s->asm_bits->cmd{,_len} */
	if(s->asm_bits->extended){
		char *str;

		for(str = s->asm_bits->cmd; *str; str++)
			if(*str == '%'){
				if(str[1] == '%'){
					str++;
				}else{
					int pos;

					if(sscanf(str + 1, "%d", &pos) != 1)
						DIE_AT(&s->where, "invalid register character '%c', number expected", str[1]);

					if(pos >= n_inouts)
						DIE_AT(&s->where, "invalid register index %d / %d", pos, n_inouts);
				}
			}
	}
}

void gen_stmt_asm(stmt *s)
{
	asm_inout **ios = s->asm_bits->inputs;
	int i;

	if(ios){
		for(i = 0; ios[i]; i++)
			gen_expr(ios[i]->exp, s->symtab);

		/* move into the registers or wherever necessary */
		for(i--; i >= 0; i--)
			out_constrain(ios[i]);
	}

	out_comment("### %sasm() from %s",
			s->asm_bits->extended ? "extended-" : "",
			where_str(&s->where));

	out_asm_inline(s->asm_bits);

	out_comment("### end asm()");

	ios = s->asm_bits->outputs;
	if(ios){
		for(i = 0; ios[i]; i++){
			asm_inout *const io = ios[i];

			lea_expr(io->exp, s->symtab);
			out_push_constrained(io);
			out_store();
		}
	}
}

void mutate_stmt_asm(stmt *s)
{
	s->f_passable = fold_passable_yes;
}
