#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "asm_out.h"
#include "asm.h"

static asm_output **asm_prog;

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

static void asm_out_generic(const char *opc, asm_output *out, int n_ops)
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
	}else{
		UCC_ASSERT(!out->lhs, "asm lhs operand found when not expected");
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
ASM_WRAP(not,      1)
ASM_WRAP(and,      2)
ASM_WRAP(or,       2)
ASM_WRAP(xor,      2)
ASM_WRAP(cmp,      2)
ASM_WRAP(test,     2)
ASM_WRAP(shl,      2)
ASM_WRAP(shr,      2)
ASM_WRAP(call,     1)
ASM_WRAP(leave,    0)
ASM_WRAP(ret,      0)
ASM_WRAP(lea,      2)
ASM_WRAP(idiv,     1)

ASM_WRAP(push,     1)

void asm_out_type_pop(asm_output *out)
{
	asm_out_generic("pop", out, 1);

	/* TODO: optimise for poping a full word
	 * if(out->lhs->tt)*/
	fprintf(cc_out[SECTION_TEXT], "; TODO: truncate ^\n");
	ICE("TODO: truncate pop");
}

void asm_out_type_mov(asm_output *out)
{
	char buf[8];

	snprintf(buf, sizeof buf, "mov%s", out->extra ? out->extra : "");

	asm_out_generic(buf, out, 2);
}

static void asm_out_type_comment(asm_output *out)
{
	fprintf(cc_out[SECTION_TEXT], "\t; %s\n", out->extra);
}

void asm_out_type_label(asm_output *out)
{
	UCC_ASSERT(out->extra && *out->extra, "no label");
	fprintf(cc_out[SECTION_TEXT], "%s:\n", out->extra);
}

void asm_out_type_jmp(asm_output *out)
{
	if(out->extra){
		/* jz %s, etc */
		fprintf(cc_out[SECTION_TEXT], "\tj%s %s\n", out->extra, out->lhs->label);
	}else{
		asm_out_generic("jmp", out, 1);
	}
}

void asm_out_type_set(asm_output *out)
{
	FILE *f = cc_out[SECTION_TEXT];

	fprintf(f, "\tset%s %s\n", out->extra, out->lhs->impl(out->lhs));

	UCC_ASSERT(!out->rhs, "asm rhs operand found when not expected");
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
	static char buf[128];
	unsigned int n;
	const char *tstr;

	/* if it's rsp or rbp, don't add "qword" and pals on */
	if(op->deref_base->impl == asm_operand_reg){
		tstr = "";
	}else{
		tstr = asm_type_str(op->tt);
	}

	n = snprintf(buf, sizeof buf, "[%s %s + %d]",
			tstr,
			op->deref_base->impl(op->deref_base),
			op->deref_offset);

	if(n >= sizeof buf)
		ICE("buffer too small for deref-asm operand");

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

asm_operand *asm_operand_new_intval(intval *iv)
{
	asm_operand *new = asm_operand_new(NULL);
	new->impl = asm_operand_val;
	new->iv = iv;
	return new;
}

asm_operand *asm_operand_new_val(int i)
{
	return asm_operand_new_intval(intval_new(i));
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
	UCC_ASSERT(deref_base, "no deref");
	new->deref_base   = deref_base;
	new->deref_offset = offset;
	return new;
}

asm_output *asm_output_new(asm_out_func *impl, asm_operand *lhs, asm_operand *rhs)
{
	asm_output *out = umalloc(sizeof *out);
	out->impl = impl;
	out->lhs = lhs;
	out->rhs = rhs;
	dynarray_add((void ***)&asm_prog, out);
	return out;
}

void asm_set(const char *cmd, enum asm_reg reg)
{
	where *old_w = eof_where;
	where w;

	decl *d;
	asm_operand *op;
	asm_output *o;

	eof_where = &w;
	memset(eof_where, 0, sizeof w);
	d = decl_new();
	eof_where = old_w;

	d->type->primitive = type_char; /* force "sete al" rather than "sete rax" */
	op = asm_operand_new_reg(d, reg);
	o = asm_output_new(asm_out_type_set, op, NULL);

	o->extra = ustrdup(cmd);
}

void asm_label(const char *lbl)
{
	asm_output *o = asm_output_new(
			asm_out_type_label,
			NULL, NULL);

	UCC_ASSERT(lbl && *lbl, "no label");

	o->extra = ustrdup(lbl);
}

void asm_jmp_custom(const char *test, const char *lbl)
{
	asm_output *o = asm_output_new(
			asm_out_type_jmp,
			asm_operand_new_label(NULL, ustrdup(lbl)),
			NULL);

	if(test)
		o->extra = ustrdup(test);
}

void asm_jmp(const char *lbl)
{
	asm_jmp_custom(NULL, lbl);
}

void asm_jmp_if_zero(int invert, const char *lbl)
{
	char buf[4];
	snprintf(buf, sizeof buf, "%sz", invert ? "n" : "");
	asm_jmp_custom(buf, lbl);
}

void asm_comment(const char *fmt, ...)
{
	va_list l;

	asm_output *o = asm_output_new(
			asm_out_type_comment,
			NULL, NULL);

	va_start(l, fmt);
	o->extra = ustrvprintf(fmt, l);
	va_end(l);
}

/* wrappers for asm_output_new */
void asm_push(enum asm_reg reg)
{
	asm_output_new(
			asm_out_type_push,
			asm_operand_new_reg(NULL, reg),
			NULL);
}

void asm_pop(decl *d, enum asm_reg reg)
{
	asm_output_new(
			asm_out_type_pop,
			asm_operand_new_reg(d, reg),
			NULL);
}

void asm_out_section(enum section_type t, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(cc_out[t], fmt, l);
	va_end(l);
	fputc('\n', cc_out[t]);
}

void asm_flush()
{
	asm_output **i;
	for(i = asm_prog; i && *i; i++){
		(*i)->impl(*i);
		// TODO: free
	}
}
