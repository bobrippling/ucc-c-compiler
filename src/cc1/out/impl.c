#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../data_structs.h"
#include "../decl.h"
#include "vstack.h"
#include "impl.h"

#include "../cc1.h"

static void out_asmv(enum p_opts opts, const char *fmt, va_list l)
{
	FILE *f = cc_out[SECTION_TEXT];

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
	out_asmv(0, fmt, l);
	va_end(l);
}

void out_asm2(enum p_opts opts, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(opts, fmt, l);
	va_end(l);
}

void impl_comment(const char *fmt, va_list l)
{
	out_asm2(P_NO_NL, IMPL_COMMENT " ");
	out_asmv(P_NO_INDENT, fmt, l);
}

void impl_lbl(const char *lbl)
{
	out_asm2(P_NO_INDENT, "%s:", lbl);
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
