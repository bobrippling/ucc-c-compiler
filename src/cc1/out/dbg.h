#ifndef DBG_H
#define DBG_H

#include "../../util/compiler.h"

struct out_dbg_filelist
{
	const char *fname;
	struct out_dbg_filelist *next;
};

/* debug output */
void out_dbg_where(out_ctx *octx, const where *w);

void dbg_out_filelist(struct out_dbg_filelist *head, FILE *f);

#ifdef SYM_H /* symtab_global */
void out_dbg_begin(
		out_ctx *octx,
		struct out_dbg_filelist **pfilelist,
		const char *fname,
		const char *compdir);

void out_dbg_end(out_ctx *octx);
#endif

void out_dbg_emit_func(out_ctx *, struct decl *);
void out_dbg_emit_global_var(out_ctx *, struct decl *);

void out_dbg_emit_decl(out_ctx *, struct decl *, const out_val *);
void out_dbg_emit_global_decl_scoped(out_ctx *, struct decl *);

/* maybe add on a variadic tag */
void out_dbg_emit_args_done(out_ctx *, struct funcargs *);

void out_dbg_scope_enter(out_ctx *, struct symtable *);
void out_dbg_scope_leave(out_ctx *, struct symtable *);

/* parameters are added during the inline code gen */
struct out_dbg_lbl;
void out_dbg_inlined_call(
		out_ctx *,
		struct decl *dinlined,
		struct out_dbg_lbl *caller_start_lbl,
		struct out_dbg_lbl *caller_end_lbl,
		const where *call_locn);

void out_dbg_inline_end(out_ctx *);

#endif
