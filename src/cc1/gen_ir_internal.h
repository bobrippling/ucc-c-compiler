#ifndef GEN_IR_INTERNAL_H
#define GEN_IR_INTERNAL_H

struct irctx
{
	unsigned curval;
	unsigned curtype;
	dynmap *structints;
};

#endif
