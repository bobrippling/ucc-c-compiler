#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/dynmap.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "vstack.h"
#include "out.h"
#include "x86_64.h"
#include "__asm.h"
#include "impl.h"
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
	size_t first_not = strspn(constraint, "gabcdSDmirqf&=");

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

				case 'n':
					const_chosen = 1;
					break;

				case 'g':
					/* any */
					ICE("TODO: any constraint");
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

		/* TODO below: allow multiple options for a constraint */
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

typedef struct
{
	enum constraint_type
	{
		C_REG,
		C_MEM,
		C_CONST,
	} type;

	int reg;
} constraint_t;

static void constraint_type(const char *constraint, constraint_t *con)
{
	int reg = -1, mem = 0, is_const = 0;

	while(*constraint)
		switch(*constraint++){
#define CHOOSE(c, i) case c: reg = i; break
			CHOOSE('a', X86_64_REG_RAX);
			CHOOSE('b', X86_64_REG_RBX);
			CHOOSE('c', X86_64_REG_RCX);
			CHOOSE('d', X86_64_REG_RDX);
#undef CHOOSE

			case 'f': ICE("TODO: fp reg constraint");

			case 'S': reg = X86_64_REG_RSI; break;
			case 'D': reg = X86_64_REG_RDI; break;

			case 'q': /* currently the same as 'r' */
			case 'r':
				reg = v_unused_reg(1);
				break;

			case 'm':
				mem = 1;
				break;
			case 'n':
				is_const = 1;
				break;

			default:
			{
				const char c = constraint[-1];
				if('0' <= c && c <= '9')
					ICE("TODO: digit/match constraint");
			}
		}

	if(mem){
		con->type = C_MEM;

	}else if(is_const){
		con->type = C_CONST;

	}else{
		con->type = C_REG;
		con->reg = reg;
	}
}

void out_constrain(asm_inout *io)
{
	/* pop into a register/memory if needed */
	constraint_t con;

	constraint_type(io->constraints, &con);

	switch(con.type){
		case C_MEM:
			/* vtop into memory */
			v_to_mem(vtop);
			break;

		case C_CONST:
			if(vtop->type != CONST)
				DIE_AT(&io->exp->where, "invalid operand for const-constraint");
			break;

		case C_REG:
		{
			const int reg = con.reg;

			v_freeup_reg(reg, 1);
			v_to_reg(vtop); /* TODO: v_to_reg_preferred */

			if(vtop->bits.reg != reg){
				impl_reg_cp(vtop, reg);
				vtop->bits.reg = reg;
			}
			break;
		}
	}
}

static void buf_add(char **pbuf, const char *s)
{
	char *buf = *pbuf;
	const int len = (buf ? strlen(buf) : 0);

	*pbuf = buf = urealloc(buf, len + strlen(s) + 1);

	strcpy(buf + len, s);
}

void out_asm_inline(asm_args *cmd)
{
	const int n_outputs = dynarray_count((void **)cmd->outputs);
	FILE *const out = cc_out[SECTION_TEXT];

	if(cmd->extended){
		int index;
		int npops = 0;
		int stack_res = 0;
		char *buf = NULL;
		char *p;
		struct vstack *const argtop = vtop;

		for(p = cmd->cmd; *p; p++){
			if(*p == '%' && *++p != '%'){
				const char *replace_str = NULL;
				struct vstack *vp;

				if(*p == '['){
					ICE("TODO: named constraint");
				}

				if(sscanf(p, "%d", &index) != 1)
					ICE("not an int - should've been caught");

				/* bounds check is already done in stmt_asm.c */
				vp = &argtop[-index];

				if(index < n_outputs){
					/* output - reserve a reg/mem and store after */
					char *constraint = cmd->outputs[index]->constraints;
					constraint_t con;

					constraint_type(constraint, &con);

					vpush(vp->t);

					switch(con.type){
						case C_REG:
							vtop->type = REG;
							vtop->bits.reg = v_unused_reg(1);
							break;

						case C_MEM:
						{
							const int sz = type_ref_size(vtop->t, NULL);

							vtop->type = STACK;
							stack_res += sz;
							vtop->bits.off_from_bp = out_alloc_stack(sz);
							break;
						}

						case C_CONST:
							ICE("invalid output const");
					}

					replace_str = vstack_str(vtop);
					npops++;
				}

				if(!replace_str)
					replace_str = vstack_str(vp);

				buf_add(&buf, replace_str);
			}else{
				char to_add[2];

				to_add[0] = *p;
				to_add[1] = '\0';

				buf_add(&buf, to_add);
			}
		}

		out_comment("### actual inline");
		fprintf(out, "\t%s\n", buf ? buf : "");
		out_comment("### end");

		index = n_outputs;
		while(npops --> 0){
			/* assign to the correct store */
			impl_store(vtop, &argtop[--index]);

			vpop();
		}
		out_free_stack(stack_res);
	}else{
		fprintf(out, "%s\n", cmd->cmd);
	}
}
