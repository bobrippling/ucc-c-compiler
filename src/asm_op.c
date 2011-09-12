#include <stdlib.h>
#include <stdio.h>

#include "tree.h"
#include "gen_asm.h"
#include "asm.h"
#include "util.h"
#include "asm_op.h"

static void asm_idiv(expr *e, symtable *tab)
{
	/*
	 * idiv — Integer Division
	 * The idiv asm_temp divides the contents of the 64 bit integer EDX:EAX (constructed by viewing EDX as the most significant four bytes and EAX as the least significant four bytes) by the specified operand value. The quotient result of the division is stored into EAX, while the remainder is placed in EDX.
	 * Syntax
	 * idiv <reg32>
	 * idiv <mem>
	 *
	 * Examples
	 *
	 * idiv ebx — divide the contents of EDX:EAX by the contents of EBX. Place the quotient in EAX and the remainder in EDX.
	 * idiv DWORD PTR [var] — divide the contents of EDX:EAS by the 32-bit value stored at memory location var. Place the quotient in EAX and the remainder in EDX.
	 */

	walk_expr(e->lhs, tab);
	walk_expr(e->rhs, tab);
	/* pop top stack (rhs) into b, and next top into a */

	asm_temp("xor rdx,rdx");
	asm_temp("pop rbx");
	asm_temp("pop rax");
	asm_temp("idiv rbx");

	if(e->op == op_divide)
		asm_temp("push rax");
	else /* modulo */
		asm_temp("push rdx");
}

static void asm_compare(expr *e, symtable *tab)
{
	const char *cmp = NULL;

	walk_expr(e->lhs, tab);
	walk_expr(e->rhs, tab);
	asm_temp("pop rbx");
	asm_temp("pop rax");
	asm_temp("xor rcx,rcx"); /* must be before cmp */
	asm_temp("cmp rax,rbx");

	switch(e->op){
		case op_eq: cmp = "e";  break;
		case op_ne: cmp = "ne"; break;
		case op_le: cmp = "le"; break;
		case op_lt: cmp = "l";  break;
		case op_ge: cmp = "ge"; break;
		case op_gt: cmp = "g";  break;

		default:
			cmp = NULL;
			break;
	}
	if(!cmp)
		DIE_ICE();

	asm_temp("set%s cl", cmp);
	asm_temp("push rcx");
}

static void asm_shortcircuit(expr *e, symtable *tab)
{
	char *baillabel = label_code("shortcircuit_bail");
	walk_expr(e->lhs, tab);

	asm_temp("mov rax,[rsp]");
	asm_temp("test rax,rax");
	/* leave the result on the stack (if false) and bail */
	asm_temp("j%sz %s", e->op == op_andsc ? "" : "n", baillabel);
	asm_temp("pop rax");
	walk_expr(e->rhs, tab);

	asm_temp(baillabel);
	free(baillabel);
}

void asm_operate(expr *e, symtable *tab)
{
	const char *instruct = NULL;

	switch(e->op){
		/* normal mafs */
		case op_multiply: instruct = "imul"; break;
		case op_plus:     instruct = "add";  break;
		case op_or:       instruct = "or";   break;
		case op_and:      instruct = "and";  break;

		/* single register op */
		case op_minus: instruct = e->rhs ? "sub" : "neg"; break;
		case op_bnot:  instruct = "not";                  break;


		case op_not:
			/* compare with 0 */
			walk_expr(e->lhs, tab);
			asm_temp("xor rbx,rbx");
			asm_temp("pop rax");
			asm_temp("test rax,rax");
			asm_temp("setz bl");
			asm_temp("push rbx");
			return;

		case op_deref:
			walk_expr(e->lhs, tab);
			asm_temp("pop rax");

			if(e->vartype->ptr_depth)
				goto ptr;
			switch(e->vartype->primitive){
				case type_char:
					asm_temp("movzx rax, byte [rax]");
					break;
ptr:
				case type_int:
				case type_void:
				case type_unknown:
					asm_temp("mov rax, [rax]");
					break;
			}
			asm_temp("push rax");
			return;

		/* comparison */
		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			asm_compare(e, tab);
			return;

		/* shortcircuit */
		case op_orsc:
		case op_andsc:
			asm_shortcircuit(e, tab);
			return;

		case op_divide:
		case op_modulus:
			asm_idiv(e, tab);
			return;

		case op_unknown:
			DIE_ICE();
	}

	/* get here if op is *, +, - or ~ */
	walk_expr(e->lhs, tab);
	if(e->rhs){
		walk_expr(e->rhs, tab);
		asm_temp("pop rbx");
		asm_temp("pop rax");
		asm_temp("%s rax, rbx", instruct);
	}else{
		asm_temp("pop rax");
		asm_temp("%s rax", instruct);
	}

	asm_temp("push rax");
}
