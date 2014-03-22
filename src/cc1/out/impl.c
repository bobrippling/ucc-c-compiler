#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"

#include "../decl.h"
#include "../op.h"
#include "../macros.h"

#include "val.h"
#include "asm.h"
#include "impl.h"
#include "write.h"

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

const char *flag_cmp_to_str(enum flag_cmp cmp)
{
	switch(cmp){
		CASE_STR_PREFIX(flag, eq);
		CASE_STR_PREFIX(flag, ne);
		CASE_STR_PREFIX(flag, le);
		CASE_STR_PREFIX(flag, lt);
		CASE_STR_PREFIX(flag, ge);
		CASE_STR_PREFIX(flag, gt);
		CASE_STR_PREFIX(flag, overflow);
		CASE_STR_PREFIX(flag, no_overflow);
	}
	return NULL;
}
