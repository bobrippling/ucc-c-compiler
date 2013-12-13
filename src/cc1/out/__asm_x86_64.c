#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/dynmap.h"
#include "../../util/alloc.h"

#include "../data_structs.h"
#include "vstack.h"
#include "asm.h"
#include "out.h"
#include "x86_64.h"
#include "__asm.h"
#include "impl.h"
#include "../cc1.h"
#include "../pack.h"

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
		die_at(w, "invalid constraint \"%c\"", constraint[first_not]);

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
		die_at(w, "bad constraint \"%s\": " err, orig)

		if(output != write_only)
			die_at(w, "%s output constraint", output ? "missing" : "unwanted");

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

struct chosen_constraint
{
	enum constraint_type
	{
		C_REG,
		C_MEM,
		C_CONST,
	} type;

	union
	{
		struct vreg reg;
		unsigned long stack;
		unsigned long k;
	} bits;
};

#define CONSTRAINT_EQ_OUT_STORE(cc, oo) (            \
	((cc) == C_REG && (oo) == V_REG) ||                \
	((cc) == C_MEM && (oo) == V_REG_SAVE))

static void out_unconstrain(const int lval_vp_offset, struct chosen_constraint *cc)
{
	/* pop from register/memory to vp */
	struct vstack *vp = &vtop[-lval_vp_offset];

	ICW("asm() storing to output doesn't work");

	switch(cc->type){
		case C_MEM:
			switch(vp->type){
				/* only certain ones are valid,
				 * may need mem->reg, reg->mem, etc */
				case V_CONST_I:
				case V_CONST_F:
				case V_FLAG:
					goto bad;

				case V_REG_SAVE:
				case V_LBL:
				case V_REG:
					/* create a mem-pointer vstack to assign to */
					vpush(type_ref_ptr_depth_inc(vp->t));

					/* FIXME: what about labels? */
					v_set_stack(vtop, NULL, cc->bits.stack, 1);

					/* vstack is:
					 * vp  { type = REG }
					 * ptr { type = STACK }
					 */
					out_store();
			}
			break;

		case C_REG:
			switch(vp->type){
				/* only certain ones are valid,
				 * may need mem->reg, reg->mem, etc */
				case V_CONST_I:
				case V_CONST_F:
				case V_FLAG:
					goto bad;

				case V_REG_SAVE:
				case V_LBL:
					/* FIXME: this doesn't work */
					vpush(vp->t);

					v_set_stack(vtop, NULL, cc->bits.stack, 1);
					/* FIXME: also what about labels? */

					impl_store(vp, vtop);
					out_pop();
					break;

				case V_REG:
					ICE("TODO");
					/*impl_reg_cp_rev(vp, cc->bits.reg);*/
			}
			break;

		case C_CONST:
bad:
			ICE("bad unconstraint type C_CONST");
	}
}

static void populate_constraint(const char *constraint, struct chosen_constraint *con)
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
				reg = -1;
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
		con->bits.reg.is_float = 0;
		con->bits.reg.idx	= reg;
	}
}

static void out_constrain(
		asm_inout *io,
		struct vstack *vp,
		struct chosen_constraint *cc,
		const int already_set,
		where *const err_w)
{
	/* pick one */
	populate_constraint(io->constraints, cc);

	if(already_set){
		if(!CONSTRAINT_EQ_OUT_STORE(cc->type, vp->type))
			die_at(err_w, "couldn't satisfy mismatching constraints");

	}else{
		/* fill it with the right values */
		switch(cc->type){
			case C_MEM:
				/* vp into memory */
				v_to_mem(vp);
				cc->bits.stack = vp->bits.regoff.offset;
				break;

			case C_CONST:
				if(vp->type != V_CONST_I)
					die_at(&io->exp->where, "invalid operand for const-constraint");
				cc->bits.k = vp->bits.val_i;
				break;

			case C_REG:
				if(cc->bits.reg.idx == (short unsigned)-1){
					if(vp->type == V_REG){
						cc->bits.reg = vp->bits.regoff.reg;
					}else{
						v_unused_reg(1, 0, &cc->bits.reg);

						v_freeup_reg(&cc->bits.reg, 1);
						v_to_reg(vp); /* TODO: v_to_reg_preferred */
					}
				}

				if(vp->bits.regoff.reg.idx != cc->bits.reg.idx){
					impl_reg_cp(vp, &cc->bits.reg);
					vp->bits.regoff.reg = cc->bits.reg;
				}
				break;
		}
	}
}

static void buf_add(char **pbuf, const char *s)
{
	char *buf = *pbuf;
	const int len = (buf ? strlen(buf) : 0);

	*pbuf = buf = urealloc1(buf, len + strlen(s) + 1);

	strcpy(buf + len, s);
}

void out_asm_inline(asm_args *cmd, where *const err_w)
{
	FILE *const out = cc_out[SECTION_TEXT];

	if(cmd->extended){
		int n_ios = dynarray_count((void **)cmd->ios);
#define IO_IDX_TO_VTOP_IDX(i)  (-n_ios + 1 + (i))
		struct vstack *const vtop_io = vtop;
		char *asm_cmd = NULL;
		char *p;
		int n_output_derefs = 0;
		struct chosen_constraint *constraints = umalloc(n_ios * sizeof *constraints);
		char  *constraint_set                 = umalloc(n_ios);

		for(p = cmd->cmd; *p; p++){
			if(*p == '%' && *++p != '%'){
				struct vstack *vp;
				int this_index;
				asm_inout *io;

				if(*p == '['){
					ICE("TODO: named constraint");
				}

				errno = 0;
				this_index = strtol(p, NULL, 0);
				if(errno)
					ICE("not an int - should've been caught");

				/* bounds check is already done in stmt_asm.c */
				io = cmd->ios[this_index];
				vp = &vtop_io[IO_IDX_TO_VTOP_IDX(this_index)];

				if(io->is_output){
					/* create a new vstack for it,
					 * which will contain the deref for now */
					vpush(vp->t);
					/* XXX: copying vstack? out_dup_from() ? */
					memcpy_safe(vtop, vp);
					/* FIXME: struct/array - we write into as much
					 * as possible, up to a machine word */
					out_deref();
					vp = vtop;
					n_output_derefs++;
				}
				out_constrain(
						io, vp,
						&constraints[this_index],
						constraint_set[this_index]++,
						err_w);

				buf_add(&asm_cmd, vstack_str(vp, 0));

			}else{
				char to_add[2];

				to_add[0] = *p;
				to_add[1] = '\0';

				buf_add(&asm_cmd, to_add);
			}
		}

		out_comment("### actual inline");
		fprintf(out, "\t%s\n", asm_cmd ? asm_cmd : "");
		out_comment("### assignments to outputs");

		/* don't care about the values we pulled now */
		while(n_output_derefs > 0)
			out_pop(), n_output_derefs--;

		/* store to the output pointers */
		{
			int i;

			for(i = 0; cmd->ios[i]; i++){
				asm_inout *io = cmd->ios[i];

				if(io->is_output){
					fprintf(stderr, "found output, index %d, expr %s, constraint %s, exists in TYPE=%d, bits=%d\n",
							i, cmd->ios[i]->exp->f_str(), cmd->ios[i]->constraints,
							constraints[i].type, constraints[i].bits.reg.idx);

					out_unconstrain(IO_IDX_TO_VTOP_IDX(i), &constraints[i]);
				}
			}
		}

		/* cleanup */
		while(n_ios > 0)
			out_pop(), n_ios--;

		free(constraints);
		free(constraint_set);
	}else{
		fprintf(out, "%s\n", cmd->cmd);
	}
}
