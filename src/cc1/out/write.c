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
#include "out.h"

#include "../cc1.h" /* cc_out */

#define DUMP_LIVE_VALS 0

static void add_live_vals(out_ctx *octx)
{
	out_val_list *l;
	int at_least_one = 0;
	char *buf;
	const char *sep;

	for(l = octx->val_head; l; l = l->next){
		if(l->val.retains > 0){
			at_least_one = 1;
			break;
		}
	}
	if(!at_least_one)
		return;

	buf = umalloc(1);
	sep = "";
	for(l = octx->val_head; l; l = l->next){
		char *old = buf;

		if(l->val.retains == 0)
			continue;

		buf = ustrprintf("%s%s%s", buf, sep, out_val_str(&l->val, 0));
		sep = ", ";
		free(old);
	}

	char *old = buf;
	buf = ustrprintf("\t/* live: %s */\n", buf);
	free(old);

	dynarray_add(&octx->current_blk->insns, buf);
}

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

	if((opts & P_NO_LIVEDUMP) == 0 && DUMP_LIVE_VALS)
		add_live_vals(octx);

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

int dbg_add_file(struct out_dbg_filelist **files, const char *nam)
{
	struct out_dbg_filelist **p;
	int i = 1; /* indexes start at 1 */

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
	int idx;

	if(!octx->dbg.where.fname || !cc1_gdebug)
		return;

	/* .file is output later */
	idx = dbg_add_file(&octx->dbg.file_head, octx->dbg.where.fname);

	if(octx->dbg.last_file == idx && octx->dbg.last_line == octx->dbg.where.line)
		return;
	/* XXX: prevents recursion as well as collapsing multiples */
	octx->dbg.last_file = idx;
	octx->dbg.last_line = octx->dbg.where.line;

	blk_add_insn(
			blk,
			ustrprintf(".loc %d %d %d\n",
				idx,
				octx->dbg.where.line,
				octx->dbg.where.chr));
}

void out_dbg_where(out_ctx *octx, const where *w)
{
	memcpy_safe(&octx->dbg.where, w);
}
