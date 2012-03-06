#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "asm_out.h"
#include "cc1.h"
#include "asm.h"

#define TODO() ICE("TODO: %s", __func__)

//static asm_output **asm_prog;

static const char *asm_reg_str(decl *d, enum asm_reg r)
{
	static char buf[8];
	const char *pre, *post;

	asm_reg_name(d, &pre, &post);

	if(r == ASM_REG_BP || r == ASM_REG_SP)
		post = "p";

	snprintf(buf, sizeof buf, "%s%c%s",
			pre,
			"abcdbs"[r],
			post);

	return buf;
}

void asm_out_generic(const char *opc, asm_output *out, int n_ops)
{
	/* for now, just output text, rather than analysis, etc */
	FILE *f = cc_out[SECTION_TEXT];

	fprintf(f, "\t%s%s", opc, n_ops ? " " : "");

	if(n_ops > 0){
		fprintf(f, "%s", out->lhs->impl(out->lhs));

		if(n_ops > 1){
			fprintf(f, ", %s", out->rhs->impl(out->rhs));
		}else{
			UCC_ASSERT(!out->rhs, "asm rhs operand found when not expected");
		}
	}

	fputc('\n', f);
}

#define ASM_WRAP(t, nops)                   \
void asm_out_type_ ## t(asm_output *out)    \
{                                           \
	asm_out_generic(#t, out, nops);           \
}

ASM_WRAP(add,      2)
ASM_WRAP(sub,      2)
ASM_WRAP(imul,     2)
ASM_WRAP(neg,      1)
ASM_WRAP(and,      2)
ASM_WRAP(or,       2)
ASM_WRAP(xor,      2)
ASM_WRAP(cmp,      2)
ASM_WRAP(test,     2)
ASM_WRAP(mov,      2)
ASM_WRAP(pop,      1)
ASM_WRAP(push,     1)
ASM_WRAP(shl,      2)
ASM_WRAP(shr,      2)
ASM_WRAP(jmp,      1)
ASM_WRAP(call,     1)
ASM_WRAP(leave,    0)
ASM_WRAP(ret,      0)
ASM_WRAP(lea,      2)

void asm_out_type_set(asm_output *out)
{
	(void)out;
	TODO();
}

void asm_out_type_idiv(asm_output *out)
{
	(void)out;
	TODO();
}

void asm_out_type_not(asm_output *out)
{
	(void)out;
	TODO();
}

static const char *asm_operand_reg(asm_operand *op)
{
	return asm_reg_str(op->tt, op->reg);
}

static const char *asm_operand_label(asm_operand *op)
{
	return op->label;
}

static const char *asm_operand_deref(asm_operand *op)
{
	static char buf[32];
	snprintf(buf, sizeof buf, "[%s + %d]",
			/*asm_type_str(op->tt),*/
			op->deref_base->impl(op->deref_base),
			op->deref_offset);
	return buf;
}

static const char *asm_operand_val(asm_operand *op)
{
	return asm_intval_str(op->iv);
}

asm_operand *asm_operand_new(decl *tt)
{
	asm_operand *new = umalloc(sizeof *new);
	new->tt = tt;
	return new;
}

asm_operand *asm_operand_new_reg(decl *tt, enum asm_reg reg)
{
	asm_operand *new = asm_operand_new(tt);
	new->impl = asm_operand_reg;
	new->reg  = reg;
	return new;
}

asm_operand *asm_operand_new_val(int i)
{
	asm_operand *new = asm_operand_new(NULL);
	new->impl = asm_operand_val;
	new->iv = intval_new(i);
	return new;
}

asm_operand *asm_operand_new_label(decl *tt, const char *lbl)
{
	asm_operand *new = asm_operand_new(tt);
	new->impl = asm_operand_label;
	new->label = lbl;
	return new;
}

asm_operand *asm_operand_new_deref(decl *tt, asm_operand *deref_base, int offset)
{
	asm_operand *new  = asm_operand_new(tt);
	new->impl         = asm_operand_deref;
	new->deref_base   = deref_base;
	new->deref_offset = offset;
	return new;
}

void asm_output_new(asm_out_func *impl, asm_operand *lhs, asm_operand *rhs)
{
	asm_output *out = umalloc(sizeof *out);
	out->impl = impl;
	out->lhs = lhs;
	out->rhs = rhs;
#ifdef ASM_CACHE
	dynarray_add((void ***)&asm_prog, out);
#else
	impl(out);
	//asm_output_free(out);
#endif
}

void asm_set(const char *cmd, enum asm_reg reg)
{
	(void)cmd;
	(void)reg;
	TODO();
}

void asm_jmp_custom(const char *test, const char *lbl)
{
	(void)test;
	(void)lbl;
	TODO();
}

void asm_jmp(const char *lbl)
{
	asm_output_new(
			asm_out_type_jmp,
			asm_operand_new_label(NULL, lbl),
			NULL);
}

void asm_jmp_if_zero(int invert, const char *lbl)
{
	(void)invert;
	(void)lbl;
	TODO(); // asm_output_new, but set with invert, etc
}

void asm_out_strv(FILE *f, const char *fmt, va_list l)
{
	vfprintf(f, fmt, l);
	fputc('\n', f);
}

void asm_out_str(FILE *f, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	asm_out_strv(f, fmt, l);
	va_end(l);
}

void asm_comment(const char *fmt, ...)
{
	FILE *f = cc_out[SECTION_TEXT];
	va_list l;

	// FIXME: ordering, for when flushing is added
	fprintf(f, "\t; ");

	va_start(l, fmt);
	asm_out_strv(f, fmt, l);
	va_end(l);
}

/* wrappers for asm_output_new */
void asm_push(enum asm_reg reg)
{
	asm_out_str(cc_out[SECTION_TEXT], "\tpush %s", asm_reg_str(NULL, reg));
}

void asm_pop(enum asm_reg reg)
{
	asm_out_str(cc_out[SECTION_TEXT], "\tpop %s", asm_reg_str(NULL, reg));
}
