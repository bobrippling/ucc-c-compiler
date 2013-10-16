#ifndef DBG_H
#define DBG_H

/* debug output */
void out_dbg_where(where *);

#ifdef SYM_H
void out_dbginfo(symtable_global *, const char *fname);
#endif

#endif
