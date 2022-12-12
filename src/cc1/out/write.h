#ifndef WRITE_H
#define WRITE_H

#include "../../util/compiler.h"
#include "forwards.h"
#include "dbg.h"
#include "asm.h"

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1,
	P_NO_LIVEDUMP = 1 << 2,
};

void out_asmv(
		out_ctx *,
		enum p_opts opts,
		const char *fmt, va_list l);

void out_asm(out_ctx *, const char *fmt, ...) ucc_printflike(2, 3);

void out_asm2(
		out_ctx *,
		enum p_opts opts,
		const char *fmt, ...) ucc_printflike(3, 4);

#endif
