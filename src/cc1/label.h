#ifndef LABEL_H
#define LABEL_H

#include "out/forwards.h"

struct label
{
	where where;
	char *spel;
	char *mustgen_spel;
	symtable *scope;
	struct stmt **jumpers; /* gotos that target us */
	struct stmt *next_stmt;
	unsigned blk;
	unsigned uses;
	unsigned complete : 1, unused : 1, doing_passable_check : 1;
};
typedef struct label label;

label *label_new(where *, char *id, int complete, symtable *scope);
void label_free(label *);

void label_cleanup(out_ctx *);

out_blk *label_getblk_octx(label *, out_ctx *);

struct irctx;
unsigned label_getblk_irctx(label *, struct irctx *);

#endif
