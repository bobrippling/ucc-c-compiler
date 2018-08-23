#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../../util/dynarray.h"
#include "../../util/where.h"
#include "../../util/alloc.h"

#include "../type.h"
#include "../num.h"

#include "asm.h"
#include "write.h"
#include "dbg.h"
#include "lbl.h"
#include "val.h"
#include "ctx.h"
#include "blk.h"

#include "../cc1.h" /* cc_out */

void out_asmv(
		out_ctx *octx,
		enum section_type sec, enum p_opts opts,
		const char *fmt, va_list l)
{
	char *insn;

	if(!octx->current_blk)
		return;

	if(sec != SECTION_TEXT){
		fprintf(stderr, "%s:%d: TODO: out_asmv() with section 0x%x",
				__FILE__, __LINE__, sec);
	}

	out_dbg_flush(octx, octx->current_blk);

	insn = ustrvprintf(fmt, l);

	if((opts & P_NO_INDENT) == 0
	|| (opts & P_NO_NL) == 0)
	{
		char *new = ustrprintf(
				"%s%s%s",
				(opts & P_NO_INDENT) == 0 ? "\t" : "",
				insn,
				(opts & P_NO_NL) == 0 ? "\n" : "");

		free(insn);

		insn = new;
	}

	dynarray_add(&octx->current_blk->insns, insn);
}

void out_asm(out_ctx *octx, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(octx, SECTION_TEXT, 0, fmt, l);
	va_end(l);
}

void out_asm2(
		out_ctx *octx,
		enum section_type sec, enum p_opts opts,
		const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(octx, sec, opts, fmt, l);
	va_end(l);
}

/* -- dbg -- */

unsigned dbg_add_file(struct out_dbg_filelist **files, const char *nam)
{
	struct out_dbg_filelist **p;
	unsigned i = 1; /* indexes start at 1 */

	for(p = files; *p; p = &(*p)->next, i++)
		if(!strcmp(nam, (*p)->fname))
			return i;

	*p = umalloc(sizeof **p);
	(*p)->fname = nam;
	return i;
}

void out_dbg_flush(out_ctx *octx, out_blk *blk)
{
	/* .file <fileidx> "<name>"
	 * .loc <fileidx> <line> <col>
	 */
	unsigned idx;
	char *location;

	if(!octx->dbg.where.fname || cc1_gdebug == DEBUG_OFF)
		return;

	/* .file is output later */
	idx = dbg_add_file(&octx->dbg.file_head, octx->dbg.where.fname);

	if(octx->dbg.last_file == idx && octx->dbg.last_line == octx->dbg.where.line)
		return;
	/* XXX: prevents recursion as well as collapsing multiples */
	octx->dbg.last_file = idx;
	octx->dbg.last_line = octx->dbg.where.line;

	if(cc1_gdebug_columninfo)
		location = ustrprintf(".loc %d %d %d\n", idx, octx->dbg.where.line, octx->dbg.where.chr);
	else
		location = ustrprintf(".loc %d %d\n", idx, octx->dbg.where.line);

	blk_add_insn(blk, location);
}

void out_dbg_where(out_ctx *octx, const where *w)
{
	memcpy_safe(&octx->dbg.where, w);
}
