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
#include "dbg_file.h"
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
		out_ctx *octx, enum p_opts opts,
		const char *fmt, va_list l)
{
	char *insn;

	if(!octx->current_blk)
		return;

	out_dbg_flush(octx);

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
	out_asmv(octx, 0, fmt, l);
	va_end(l);
}

void out_asm2(
		out_ctx *octx,
		enum p_opts opts,
		const char *fmt,
		...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(octx, opts, fmt, l);
	va_end(l);
}
