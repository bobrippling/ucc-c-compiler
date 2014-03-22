#ifndef DBG_H
#define DBG_H

/* debug output */
void out_dbg_where(where *);
void out_dbg_label(const char *);

#ifdef SYM_H
void out_dbginfo(symtable_global *globs,
		const char *fname,
		const char *compdir);
#endif

#endif
