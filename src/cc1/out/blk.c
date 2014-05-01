#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../../util/dynmap.h"

#include "../type.h"
#include "../num.h"

#include "val.h"
#include "ctx.h"
#include "out.h" /* out_blk_new() */
#include "lbl.h"

#include "blk.h"
#include "impl_jmp.h"

#include "asm.h" /* cc_out */

struct flush_state
{
	FILE *f;
	dynmap *pending; /* out_blk* => NULL */

	/* for jump threading - the block we jump to if not immediately flushing */
	out_blk *jmpto;
};

static char present, done;

static void bfs_add(dynmap *pending, out_blk *blk)
{
	char *v = dynmap_get(out_blk *, char *, pending, blk);
	if(!v)
		(void)dynmap_set(out_blk *, char *, pending, blk, &present);
	else if(v == &present)
		(void)dynmap_set(out_blk *, char *, pending, blk, &done);
	else
		; /* already handled this, don't flush again */
}

static void bfs_rm(dynmap *pending, out_blk *blk)
{
	(void)dynmap_set(out_blk *, char *, pending, blk, &done);
}

static void blk_jmpnext(out_blk *to, struct flush_state *st)
{
	assert(!st->jmpto);
	st->jmpto = to;
}

static void flush_block(out_blk *blk, struct flush_state *st)
{
	char **i;

	bfs_rm(st->pending, blk);

	/* before any instructions, if we have a pending jmpto and
	 * we aren't the target branch, we need to cut off the last
	 * block with a jump to said jmpto */
	if(st->jmpto){
		if(st->jmpto != blk)
			impl_jmp(st->f, st->jmpto->lbl);
		else
			fprintf(st->f, "\t# implicit jump to next line\n");
		st->jmpto = NULL;
	}

	fprintf(st->f, "%s: # %s\n", blk->lbl, blk->desc);

	for(i = blk->insns; i && *i; i++)
		fprintf(st->f, "%s", *i);

	switch(blk->type){
		case BLK_UNINIT:
			assert(0 && "uninitialised block type");
		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
			break;

		case BLK_NEXT_BLOCK:
			blk_jmpnext(blk->bits.next, st);
			bfs_add(st->pending, blk->bits.next);
			break;

		case BLK_COND:
			fprintf(st->f, "\t%s\n", blk->bits.cond.insn);
			blk_jmpnext(blk->bits.cond.if_1_blk, st);

			bfs_add(st->pending, blk->bits.cond.if_1_blk);
			bfs_add(st->pending, blk->bits.cond.if_0_blk);
			break;
	}
}

static unsigned blk_hash(const void *v)
{
	const out_blk *b = v;

	return b->type
		^ (unsigned)(unsigned long)b->desc
		^ dynarray_count(b->insns);
}

void blk_flushall(out_ctx *octx)
{
	struct flush_state st = { 0 };
	out_blk *current;
	size_t i;

	st.pending = dynmap_new(/*refeq:*/NULL, blk_hash);

	st.f = cc_out[SECTION_TEXT];
	bfs_add(st.pending, octx->first_blk);

	for(i = 0; (current = dynmap_key(out_blk *, st.pending, i)); ){
		char *p = dynmap_get(out_blk *, char *, st.pending, current);

		if(p == &present){
			flush_block(current, &st);
			i = 0;
		}else{
			i++;
		}
	}

	if(st.jmpto)
		impl_jmp(st.f, st.jmpto->lbl);

	dynmap_free(st.pending);
}

void blk_terminate_condjmp(
		out_ctx *octx, char *condinsn,
		out_blk *condto, out_blk *uncondto)
{
	out_blk *current = octx->current_blk;

	assert(current->type == BLK_UNINIT && "overwriting .next?");

	current->type = BLK_COND;

	current->bits.cond.insn = condinsn;
	current->bits.cond.if_0_blk = condto;
	current->bits.cond.if_1_blk = uncondto;

	octx->current_blk = NULL;
}

out_blk *out_blk_new_lbl(out_ctx *octx, const char *lbl)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->desc = lbl;
	blk->lbl = ustrdup(lbl);
	return blk;
}

out_blk *out_blk_new(out_ctx *octx, const char *desc)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->desc = desc;
	blk->lbl = out_label_bblock(octx->nblks++);
	return blk;
}
