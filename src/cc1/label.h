#ifndef LABEL_H
#define LABEL_H

#include "out/forwards.h"

struct label
{
	where *pw;
	char *spel;
	out_blk *bblock;
	unsigned uses;
	unsigned complete : 1, unused : 1;
};
typedef struct label label;

label *label_new(where *, char *id, int complete);
void label_makeblk(label *, out_ctx *);

#endif
