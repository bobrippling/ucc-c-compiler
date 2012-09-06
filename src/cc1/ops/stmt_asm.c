#include <string.h>

#include "ops.h"
#include "stmt_asm.h"

#if 0
+---+--------------------+
| r |    Register(s)     |
+---+--------------------+
| a |   %eax, %ax, %al   |
| b |   %ebx, %bx, %bl   |
| c |   %ecx, %cx, %cl   |
| d |   %edx, %dx, %dl   |
| S |   %esi, %si        |
| D |   %edi, %di        |
+---+--------------------+

 m  |   memory
 i  |   integral
 r  |   any reg
 q  |   reg [abcd]
 f  |   fp reg
 &  |   pre-clobber


 =  | write-only - needed in output

http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s4

#endif

const char *str_stmt_asm()
{
	return "asm";
}

static char *constraint_reg(char c)
{
	switch(c){
#define MAP(c, s) case c: return s
		MAP('a', "rax");
		MAP('b', "rbx");
		MAP('c', "rcx");
		MAP('d', "rdx");
		MAP('S', "rsi");
		MAP('D', "rdi");
#undef MAP
	}

	return NULL;
}

static void check_constraint(asm_inout *io, symtable *stab)
{
	char *c = io->constraints;

	fold_expr(io->exp, stab);

	/* constraints current can be empty, or a register name */
	if(c[1] || !constraint_reg(*c))
		DIE_AT(&io->exp->where, "invalid constraint \"%c\"", *c);
}

void fold_stmt_asm(stmt *s)
{
	char *const str = s->asm_bits->cmd;
	asm_inout **it;
	int i;
	int n_inouts;

	n_inouts = 0;

	for(it = s->asm_bits->inputs; it && *it; it++, n_inouts++)
		check_constraint(*it, s->symtab);

	for(it = s->asm_bits->outputs; it && *it; it++, n_inouts++){
		asm_inout *io = *it;
		check_constraint(io, s->symtab);
		if(!expr_is_lvalue(io->exp, 0))
			DIE_AT(&io->exp->where, "asm output not an lvalue");
	}

	/* validate asm string - s->asm_bits->cmd{,_len} */
	for(i = 0; i < s->asm_bits->cmd_len; i++){
		if(str[i] == '%'){
			if(str[i + 1] == '%'){
				i++;
			}else{
				int pos;
				if(sscanf(str + i + 1, "%d", &pos) != 1)
					DIE_AT(&s->where, "invalid register character '%c', number expected", str[i + 1]);
				if(pos >= n_inouts)
					DIE_AT(&s->where, "invalid register index %d / %d", pos, n_inouts);
			}
		}
	}
}

static void asm_filter(const char *cmd)
{
	char *s;

	for(s = strchr(cmd, '%'); s; s = strchr(s + 1, '%')){
		if(s[1] != '%'){
			fprintf(stderr, "TODO: %s\n", s);
		}
	}
}

void gen_stmt_asm(stmt *s)
{
	asm_inout **ios = s->asm_bits->inputs;
	int i;

	if(ios){
		for(i = 0; ios[i]; i++){
			asm_inout *io = ios[i];
			char *reg;

			gen_expr(io->exp, s->symtab);

			reg = constraint_reg(*io->constraints);
			if(!reg){
				/* TODO: pick one */
				ICE("TODO: pick a register for __asm__");
			}
		}

		/* move into the registers */
		for(i--; i >= 0; i--){
			const char rc = *ios[i]->constraints;

			asm_temp(1, "pop %s ; expr %s -> reg %c",
					constraint_reg(rc),
					ios[i]->exp->f_str(),
					rc);
		}
	}

	asm_filter(s->asm_bits->cmd, regs);

	ios = s->asm_bits->outputs;
	if(ios){
		for(i = 0; ios[i]; i++){
			asm_inout *io = ios[i];

			asm_temp(1, "mov rax, %s", constraint_reg(*io->constraints));
			io->exp->f_store(io->exp, s->symtab);
		}
	}
}
