#ifndef OUT_DBG_LBL_H
#define OUT_DBG_LBL_H

#include "forwards.h"
#include "../retain.h"
#include "../../util/compiler.h"

struct section;

struct out_dbg_lbl
{
	struct retain rc;
	char *lbl;
	int emitted;
};

/* TODO: this should be called at the _start_ of a basic block
 * (for the start label to effectively pinpoint the pseudo-call */
void out_dbg_label_push(
		out_ctx *octx, char *lbl[ucc_static_param 2],
		struct out_dbg_lbl **startlbl,
		struct out_dbg_lbl **endlbl);

void out_dbg_label_pop(out_ctx *octx, struct out_dbg_lbl *);


int out_dbg_label_emitted(struct out_dbg_lbl *, const char **out_lbl);
int out_dbg_label_shouldemit(struct out_dbg_lbl *, const char **out_lbl);

void out_dbg_labels_emit_release_v(struct out_dbg_lbl ***, const struct section *sec);

#endif
