#ifndef WRITE_H
#define WRITE_H

#include "../../util/compiler.h"

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1
};

void out_asmv(
		enum section_type sec,
		enum p_opts opts,
		const char *fmt, va_list l);

void out_asm(const char *fmt, ...) ucc_printflike(1, 2);

void out_asm2(enum section_type,
		enum p_opts opts,
		const char *fmt, ...) ucc_printflike(3, 4);

#endif
