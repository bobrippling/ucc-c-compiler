#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../data_structs.h"
#include "../decl.h"
#include "vstack.h"
#include "asm.h"
#include "impl.h"

#include "../cc1.h"

static void out_asmv(
		enum section_type sec, enum p_opts opts,
		const char *fmt, va_list l)
{
	FILE *f = cc_out[sec];

	if((opts & P_NO_INDENT) == 0)
		fputc('\t', f);

	vfprintf(f, fmt, l);

	if((opts & P_NO_NL) == 0)
		fputc('\n', f);
}

void out_asm(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(SECTION_TEXT, 0, fmt, l);
	va_end(l);
}

static void out_asm2(
		enum section_type sec, enum p_opts opts,
		const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(sec, opts, fmt, l);
	va_end(l);
}

void impl_comment(enum section_type sec, const char *fmt, va_list l)
{
	out_asm2(sec, P_NO_NL, "/* ");
	out_asmv(sec, P_NO_INDENT | P_NO_NL, fmt, l);
	out_asm2(sec, P_NO_INDENT, " */");
}

void impl_lbl(const char *lbl)
{
	out_asm2(SECTION_TEXT, P_NO_INDENT, "%s:", lbl);
}

enum flag_cmp op_to_flag(enum op_type op)
{
	switch(op){
#define OP(x) case op_ ## x: return flag_ ## x
		OP(eq);
		OP(ne);
		OP(le);
		OP(lt);
		OP(ge);
		OP(gt);
#undef OP

		default:
			break;
	}

	ICE("invalid op");
	return -1;
}

int vreg_eq(const struct vreg *a, const struct vreg *b)
{
	return a->idx == b->idx && a->is_float == b->is_float;
}
