#ifndef DBG_H
#define DBG_H

struct out_dbg_filelist
{
	const char *fname;
	struct out_dbg_filelist *next;
};

/* debug output */
void out_dbg_where(out_ctx *octx, where *w);
void out_dbg_label(out_ctx *octx, const char *lbl);

#ifdef SYM_H
void out_dbginfo(symtable_global *globs,
		struct out_dbg_filelist **pfilelist,
		const char *fname,
		const char *compdir);
#endif

#endif
