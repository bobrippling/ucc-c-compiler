#ifndef OUT_DBG_LBL_H
#define OUT_DBG_LBL_H

#include "forwards.h"
#include "../retain.h"
#include "../../util/compiler.h"

struct out_dbg_lbl
{
	struct retain rc;
	char *lbl;
	int emitted;
};

void out_dbg_label_push(
		out_ctx *octx, char *lbl[ucc_static_param 2],
		struct out_dbg_lbl **);

void out_dbg_label_pop(out_ctx *octx, struct out_dbg_lbl *);


int out_dbg_label_shouldemit(struct out_dbg_lbl *, const char **out_lbl);

void out_dbg_labels_emit_v(FILE *, struct out_dbg_lbl **);

#endif
