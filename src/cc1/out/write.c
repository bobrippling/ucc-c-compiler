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
#include "../str.h" /* str_add_escape */

void out_asmv(
		out_ctx *octx,
		enum section_type sec, enum p_opts opts,
		const char *fmt, va_list l)
{
	char *insn;

	if(sec != SECTION_TEXT){
		fprintf(stderr, "%s:%d: TODO: out_asmv() with section 0x%x",
				__FILE__, __LINE__, sec);
	}

	assert(octx->current_blk);

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

int dbg_add_file(struct out_dbg_filelist **files, const char *nam, int *new)
{
	struct out_dbg_filelist **p;
	int i = 1; /* indexes start at 1 */

	if(new)
		*new = 0;

	for(p = files; *p; p = &(*p)->next, i++)
		if(!strcmp(nam, (*p)->fname))
			return i;

	if(new)
		*new = 1;
	*p = umalloc(sizeof **p);
	(*p)->fname = nam;
	return i;
}

void out_dbg_flush(out_ctx *octx, out_blk *blk)
{
	/* .file <fileidx> "<name>"
	 * .loc <fileidx> <line> <col>
	 */
	int idx, new;

	if(!octx->dbg.where.fname || !cc1_gdebug)
		return;

	idx = dbg_add_file(&octx->dbg.file_head, octx->dbg.where.fname, &new);

	if(octx->dbg.last_file == idx && octx->dbg.last_line == octx->dbg.where.line)
		return;
	/* XXX: prevents recursion as well as collapsing multiples */
	octx->dbg.last_file = idx;
	octx->dbg.last_line = octx->dbg.where.line;

	/* TODO: escape w->fname */
	if(new){
		char *esc = str_add_escape(
				octx->dbg.where.fname, strlen(octx->dbg.where.fname));

		blk_add_insn(blk, ustrprintf(".file %d \"%s\"\n", idx, esc));
		free(esc);
	}

	blk_add_insn(
			blk,
			ustrprintf(".loc %d %d %d\n",
				idx,
				octx->dbg.where.line,
				octx->dbg.where.chr,
				octx->dbg.where.fname));
}

void out_dbg_where(out_ctx *octx, where *w)
{
	memcpy_safe(&octx->dbg.where, w);
}
