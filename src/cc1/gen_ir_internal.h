#ifndef GEN_IR_INTERNAL_H
#define GEN_IR_INTERNAL_H

struct irctx
{
	dynmap *sus; /* struct_union_enum_st* => char* */
	unsigned curval;
	unsigned curlbl;
};

#endif
