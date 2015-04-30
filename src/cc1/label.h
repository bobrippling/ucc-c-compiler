#ifndef LABEL_H
#define LABEL_H

#include "out/forwards.h"

struct label
{
	where *pw;
	char *spel;
	char *mustgen_spel;
	symtable *scope;
	struct stmt **jumpers; /* gotos that target us */
	struct stmt *next_stmt;
	unsigned uses;
	unsigned complete : 1, unused : 1, doing_passable_check : 1;
};
typedef struct label label;

label *label_new(where *, char *id, int complete, symtable *scope);
void label_free(label *);

out_blk *label_getblk(label *, out_ctx *);
void label_cleanup(out_ctx *);

#endif
