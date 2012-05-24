#include "dynhash.h"

#define HASH_TBL_SIZE 16

struct dynhash
{
	dynhash_func *hash;
	dynhash_ent *entries[HASH_TBL_SIZE];
};

dynhash *dynhash_new(int (*hash)(void *))
{
	dynhash *h = umalloc(sizeof *h);
	h->hash = hash;
	return h;
}

void dynhash_add(dynhash *h, void *item)
{
	const int pos = h->hash(item);
	dynhash_ent **ins;

	for(ins = &h->entries[pos]; *ins; ins = &(*ins)->next);

	ent = umalloc(sizeof *ent);
	ent->p = item;

	*ins = ent;
}
