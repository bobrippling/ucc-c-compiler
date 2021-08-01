#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../../util/alloc.h"
#include "../../util/dynarray.h"

#include "../type.h"
#include "../num.h"
#include "../str.h" /* str_add_escape */

#include "val.h"
#include "ctx.h"
#include "out.h" /* out_blk_new() */
#include "lbl.h"
#include "dbg.h"
#include "dbg_lbl.h"

#include "blk.h"
#include "impl_jmp.h"

#include "asm.h" /* cc_out */
#include "../cc1.h" /* fopt_mode */
#include "../fopt.h"

#define JMP_THREAD_LIM 10

struct flush_state
{
	/* for jump threading - the block we jump to if not immediately flushing */
	out_blk *jmpto;
};

static void dot_blocks(out_blk *);

static void blk_jmpnext(out_blk *to, struct flush_state *st)
{
	assert(!st->jmpto);
	st->jmpto = to;
}

static void blk_jmpthread(struct flush_state *st, const struct section *sec)
{
	out_blk *to = st->jmpto;

	if(cc1_fopt.thread_jumps){
		int lim = 0;

		while(!to->insns && to->type == BLK_NEXT_BLOCK && lim < JMP_THREAD_LIM){
			to = to->bits.next;
			lim++; /* prevent circulars */
		}

		if(lim && cc1_fopt.verbose_asm)
			asm_out_section(sec, "\t# jump threaded through %d blocks\n", lim);
	}

	impl_jmp(to->lbl, sec);
}

static void blk_codegen(out_blk *blk, struct flush_state *st, const struct section *sec)
{
	char **i;

	/* before any instructions, if we have a pending jmpto and
	 * we aren't the target branch, we need to cut off the last
	 * block with a jump to said jmpto */
	if(st->jmpto){
		if(st->jmpto != blk)
			blk_jmpthread(st, sec);
		else if(cc1_fopt.verbose_asm)
			asm_out_section(sec, "\t# implicit jump to next line\n");
		st->jmpto = NULL;
	}

	if(blk->align)
		asm_out_align(sec, blk->align);

	asm_out_section(sec, "%s: # %s\n", blk->lbl, blk->desc);
	if(blk->force_lbl)
		asm_out_section(sec, "%s: # mustgen_spel\n", blk->force_lbl);

	out_dbg_labels_emit_release_v(&blk->labels.start, sec);

	for(i = blk->insns; i && *i; i++)
		asm_out_section(sec, "%s", *i);

	out_dbg_labels_emit_release_v(&blk->labels.end, sec);
}

static void bfs_block(out_blk *blk, struct flush_state *st, const struct section *sec)
{
	if(blk->emitted || !blk->reachable)
		return;
	blk->emitted = 1;

	if(blk->merge_preds){
		out_blk **i;
		for(i = blk->merge_preds; *i; i++){
			bfs_block(*i, st, sec);
		}
	}

	switch(blk->type){
		case BLK_UNINIT:
			assert(0 && "uninitialised block type");

		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
		case BLK_NEXT_BLOCK:
			blk_codegen(blk, st, sec);

			if(blk->type == BLK_NEXT_BLOCK){
				blk_jmpnext(blk->bits.next, st);
				bfs_block(blk->bits.next, st, sec);
			}
			break;

		case BLK_COND:
			blk_codegen(blk, st, sec);
			asm_out_section(sec, "\t%s\n", blk->bits.cond.insn);

			/* we always jump to the true block if the conditional failed */
			blk_jmpnext(blk->bits.cond.if_1_blk, st);

			/* if it's unlikely, we want the false block already in the pipeline */
			if(blk->bits.cond.unlikely){
				bfs_block(blk->bits.cond.if_0_blk, st, sec);
				bfs_block(blk->bits.cond.if_1_blk, st, sec);
			}else{
				bfs_block(blk->bits.cond.if_1_blk, st, sec);
				bfs_block(blk->bits.cond.if_0_blk, st, sec);
			}
			break;
	}
}

static void mark_reachable_blocks(out_blk *blk)
{
	if(blk->reachable)
		return;
	blk->reachable = 1;
	switch(blk->type){
		case BLK_UNINIT:
			assert(0);
		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
			break;
		case BLK_NEXT_BLOCK:
			mark_reachable_blocks(blk->bits.next);
			break;
		case BLK_COND:
			mark_reachable_blocks(blk->bits.cond.if_0_blk);
			mark_reachable_blocks(blk->bits.cond.if_1_blk);
			break;
	}
}

void blk_flushall(out_ctx *octx, out_blk *first, char *end_dbg_lbl, const struct section *sec)
{
	struct flush_state st = { 0 };
	out_blk **must_i;

	if(cc1_fopt.dump_basic_blocks)
		dot_blocks(first);

	mark_reachable_blocks(first);
	for(must_i = octx->mustgen; must_i && *must_i; must_i++)
		mark_reachable_blocks(*must_i);

	bfs_block(first, &st, sec);

	for(must_i = octx->mustgen; must_i && *must_i; must_i++)
		bfs_block(*must_i, &st, sec);

	if(st.jmpto)
		impl_jmp(st.jmpto->lbl, sec);

	asm_out_section(sec, "%s:\n", end_dbg_lbl);

	out_dbg_labels_emit_release_v(&octx->pending_lbls, sec);
}

static void dot_replace(char *lbl)
{
	for(; *lbl; lbl++)
		if(*lbl == '.')
			*lbl = '_';
}

static void dot_emit(const char *from, const char *to)
{
	char *from_ = ustrdup(from);
	char *to_ = ustrdup(to);

	dot_replace(from_);
	dot_replace(to_);

	fprintf(stderr, "%s -> %s;\n", from_, to_);

	free(from_);
	free(to_);
}

static void dot_block(out_blk *b)
{
	if(b->emitted)
		return;
	b->emitted = 1;

	switch(b->type){
		case BLK_UNINIT:
			assert(0);

		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
			break;

		case BLK_NEXT_BLOCK:
			dot_emit(b->lbl, b->bits.next->lbl);
			dot_block(b->bits.next);
			break;

		case BLK_COND:
			dot_emit(b->lbl, b->bits.cond.if_0_blk->lbl);
			dot_emit(b->lbl, b->bits.cond.if_1_blk->lbl);
			dot_block(b->bits.cond.if_0_blk);
			dot_block(b->bits.cond.if_1_blk);
			break;
	}
}

static void dot_blocks(out_blk *b)
{
	fprintf(stderr, "digraph blocks {\n");

	dot_block(b);

	fprintf(stderr, "}\n");
}

void blk_terminate_condjmp(
		out_ctx *octx, char *condinsn,
		out_blk *condto, out_blk *uncondto,
		int unlikely)
{
	out_blk *current = octx->current_blk;

	if(!current){
		free(condinsn);
		return;
	}

	assert(current->type == BLK_UNINIT && "overwriting .next?");

	current->type = BLK_COND;

	current->bits.cond.insn = condinsn;
	current->bits.cond.if_0_blk = condto;
	current->bits.cond.if_1_blk = uncondto;
	current->bits.cond.unlikely = unlikely;

	octx->current_blk = NULL;
}

void blk_terminate_undef(out_blk *b)
{
	if(b)
		b->type = BLK_TERMINAL;
}

static out_blk *blk_new_common(out_ctx *octx, char *lbl, const char *desc)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->lbl = lbl;
	blk->desc = desc;

	blk->next = octx->mem_blk_head;
	octx->mem_blk_head = blk;

	return blk;
}

void blk_transfer(out_blk *from, out_blk *to)
{
	free(to->lbl);
	to->lbl = from->lbl;
	to->align = from->align;
	from->lbl = NULL;
	from->align = 0;
}

out_blk *out_blk_new_lbl(out_ctx *octx, const char *lbl)
{
	return blk_new_common(octx, ustrdup(lbl), lbl);
}

out_blk *out_blk_new(out_ctx *octx, const char *desc)
{
	return blk_new_common(octx, out_label_bblock(octx->nblks++), desc);
}

out_blk *out_blk_entry(out_ctx *octx)
{
	return octx->entry_blk;
}

out_blk *out_blk_postprologue(out_ctx *octx)
{
	return octx->postprologue_blk;
}

void out_blk_mustgen(out_ctx *octx, out_blk *blk, char *force_lbl)
{
	if(force_lbl)
		blk->force_lbl = force_lbl;

	dynarray_add(&octx->mustgen, blk);
}
