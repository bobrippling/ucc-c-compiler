#ifndef WRITE_H
#define WRITE_H

#include "../../util/compiler.h"
#include "forwards.h"
#include "dbg.h"

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1,
	P_NO_LIVEDUMP = 1 << 2,
};

void out_asmv(
		out_ctx *,
		enum section_type sec,
		enum p_opts opts,
		const char *fmt, va_list l);

void out_asm(out_ctx *, const char *fmt, ...) ucc_printflike(2, 3);

void out_asm2(
		out_ctx *,
		enum section_type,
		enum p_opts opts,
		const char *fmt, ...) ucc_printflike(4, 5);

void out_dbg_flush(out_ctx *, out_blk *);

int dbg_add_file(struct out_dbg_filelist **files, const char *nam);

#endif
