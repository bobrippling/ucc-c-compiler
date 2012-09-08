#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../data_structs.h"
#include "vstack.h"
#include "out.h"
#include "x86_64.h"
#include "__asm.h"
#include "../cc1.h"

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

  m |   memory
  i |   integral
  r |   any reg
  q |   reg [abcd]
  f |   fp reg
  & |   pre-clobber


  = | write-only - needed in output

http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s4

#endif

void out_constraint_check(where *w, const char *constraint, int output)
{
	const char *const orig = constraint;
	/* currently x86 specific */
	size_t first_not = strspn(constraint, "abcdSDmirqf&=");

	if(first_not != strlen(constraint))
		DIE_AT(w, "invalid constraint \"%c\"", constraint[first_not]);

	{
		char reg_chosen, mem_chosen, write_only, const_chosen;

		reg_chosen = mem_chosen = write_only = const_chosen = 0;

		while(*constraint)
			switch(*constraint++){
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'S':
				case 'D':
				case 'f':
				case 'q':
				case 'r':
					reg_chosen++;
					break;

				case 'm':
					mem_chosen = 1;
					break;

				case 'i':
					const_chosen = 1;
					break;

				case '=':
					write_only = 1;
					break;
			}

#define BAD_CONSTRAINT(err) \
		DIE_AT(w, "bad constraint \"%s\": " err, orig)

		if(output != write_only)
			DIE_AT(w, "%s output constraint", output ? "missing" : "unwanted");

		if(output && const_chosen)
			BAD_CONSTRAINT("can't output to a constant");

		if(reg_chosen > 1)
			BAD_CONSTRAINT("too many registers");

		switch(reg_chosen + mem_chosen + const_chosen){
			case 0:
				if(output)
					BAD_CONSTRAINT("constraint specifies none of memory/register/const");
				/* fall */

			case 1:
				/* fine - exactly one */
				break;

			default:
				BAD_CONSTRAINT("constraint specifies memory/register/const combination");
		}
	}
}

void out_constrain(asm_inout *io)
{
	/* pop into a register/memory if needed */
	const char *constraint = io->constraints;
	int reg = -1, mem = 0, is_const = 0;

	while(*constraint)
		switch(*constraint++){
#define CHOOSE(c, i) case c: reg = i; break
				CHOOSE('a', REG_A);
				CHOOSE('b', REG_B);
				CHOOSE('c', REG_C);
				CHOOSE('d', REG_D);
#undef CHOOSE

				case 'f': ICE("TODO: fp reg constraint");

				case 'S': reg = REG_LAST + 1; break;
				case 'D': reg = REG_LAST + 2; break;

				case 'q': /* currently the same as 'r' */
				case 'r':
					reg = v_unused_reg(1);
					break;

				case 'm':
					mem = 1;
					break;
				case 'i':
					is_const = 1;
					break;
		}

	if(mem){
		/* vtop into memory */
		v_to_mem(vtop);

	}else if(is_const){
		if(vtop->type != CONST)
			DIE_AT(&io->exp->where, "invalid operand for const-constraint");

	}else{
		const int r = reg != -1 && reg <= REG_LAST ? reg : v_unused_reg(1);

		v_freeup_reg(r, 1);
		v_to_reg(vtop);

		if(reg > REG_LAST)
			ICE("TODO: register %d for constraint", reg);

		if(vtop->bits.reg != r){
			impl_reg_cp(vtop, r);
			vtop->bits.reg = r;
		}
	}
}

void out_push_constrained(asm_inout *io)
{
	/* look for a reg or mem */
#if 0
	char *p;
	for(p = io->constraints; *p; p++)
		switch(*p){
				CHOOSE('a', REG_A);
				CHOOSE('b', REG_B);
				CHOOSE('c', REG_C);
				CHOOSE('d', REG_D);

				/* TODO */
				case 'S': reg = REG_LAST + 1; break;
				case 'D': reg = REG_LAST + 2; break;

				case 'q': /* currently the same as 'r' */
				case 'r':
					reg = v_unused_reg(1);
					break;

				case 'f': ICE("TODO: fp reg constraint");

				case 'm':
					ICE("TODO: memory output constraint");
		}
#endif

	ICE("TODO: output constraint rvalue push");
}

void out_asm_inline(asm_args *cmd)
{
	FILE *const out = cc_out[SECTION_TEXT];
	int i;

	if(cmd->extended){
		char *p;

		for(p = cmd->cmd; *p; p++){
			if(*p == '%'){
				int index;

				if(*++p == '%')
					goto normal;

				if(sscanf(p, "%d", &index) != 1)
					ICE("not an int - should've been caught");

				/* bounds check is already done in stmt_asm.c */
				fprintf(out, "%s", vstack_str(&vtop[-index]));
			}else{
	normal:
				fputc(*p, out);
			}
		}

		while(*p)
			fputc(*p, out);
		fputc('\n', out);

	}else{
		fprintf(out, "%s\n", cmd->cmd);
	}

	/* pop inputs, outputs are done by store-pops */
	for(i = dynarray_count((void **)cmd->inputs); i > 0; i--)
		vpop();
}
