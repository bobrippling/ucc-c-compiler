#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../../util/io.h"

#include "../type.h"
#include "../cc1.h"
#include "../str.h"

#include "val.h"
#include "blk.h"
#include "ctx.h"
#include "dbg_file.h"

#include "asm.h"
#include "../cc1_out.h"

unsigned dbg_add_file(struct out_dbg_filelist **files, const char *nam)
{
	struct out_dbg_filelist **p;
	unsigned i = 1; /* indexes start at 1 */
	struct cstring local;
	char *esc;

	for(p = files; *p; p = &(*p)->next, i++)
		if(!strcmp(nam, (*p)->fname))
			return i;

	*p = umalloc(sizeof **p);
	(*p)->fname = nam;

	cstring_init(&local, CSTRING_ASCII, nam, strlen(nam), 0);
	esc = str_add_escape(&local);

	xfprintf(cc1_output.file, ".file %u \"%s\"\n", i, esc);

	cstring_deinit(&local);
	free(esc);

	return i;
}

static int update_dbg_location(out_ctx *octx, unsigned fileidx, unsigned lineno, unsigned col)
{
	if(octx->dbg.last_file == fileidx && octx->dbg.last_line == lineno && octx->dbg.last_col == col)
		return 0;

	octx->dbg.last_file = fileidx;
	octx->dbg.last_line = lineno;
	octx->dbg.last_col = col;
	return 1;
}

void out_dbg_flush(out_ctx *octx)
{
	/* .file <fileidx> "<name>"
	 * .loc <fileidx> <line> <col>
	 */
	unsigned idx;
	char *location;

	if(!octx->dbg.where.fname
	|| cc1_gdebug == DEBUG_OFF
	|| !octx->current_blk)
	{
		return;
	}

	idx = dbg_add_file(&octx->dbg.file_head, octx->dbg.where.fname);

	/* XXX: prevents recursion as well as collapsing multiples */
	if(!update_dbg_location(octx, idx, octx->dbg.where.line, octx->dbg.where.chr))
		return;

	if(cc1_gdebug_columninfo)
		location = ustrprintf(".loc %d %d %d\n", idx, octx->dbg.where.line, octx->dbg.where.chr + 1);
	else
		location = ustrprintf(".loc %d %d\n", idx, octx->dbg.where.line);

	blk_add_insn(octx->current_blk, location);
}

void out_dbg_where(out_ctx *octx, const where *w)
{
	if(octx)
		memcpy_safe(&octx->dbg.where, w);
}

/*
void dbg_out_filelist(struct out_dbg_filelist *head)
{
	struct out_dbg_filelist *i;
	unsigned idx;

	for(i = head, idx = 1; i; i = i->next, idx++){
		struct cstring local;
		char *esc;

		cstring_init(&local, CSTRING_ASCII, i->fname, strlen(i->fname), 0);

		esc = str_add_escape(&local);

		asm_out_section(&section_dbg_info, ".file %u \"%s\"\n", idx, esc);

		cstring_deinit(&local);
		free(esc);
	}
}
*/
