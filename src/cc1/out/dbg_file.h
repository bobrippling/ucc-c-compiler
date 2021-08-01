#ifndef DBG_FILE_H
#define DBG_FILE_H

struct out_dbg_filelist
{
	const char *fname;
	struct out_dbg_filelist *next;
};

void out_dbg_where(out_ctx *octx, const where *w);

void out_dbg_flush(out_ctx *);

unsigned dbg_add_file(struct out_dbg_filelist **files, const char *nam);

void dbg_out_filelist(struct out_dbg_filelist *head);

#endif
