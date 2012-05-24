#ifndef DYNHASH_H
#define DYNHASH_H

#error dynhash incomplete

typedef struct dynhash     dynhash;
typedef struct dynhash_ent dynhash_ent;

typedef int dynhash_func(void *);

struct dynhash_ent
{
	void *p;
	dynhash_ent *next;
};

dynhash *dynhash_new(dynhash_func *);

void dynhash_add(dynhash *, void *);

dynhash_ent *dynhash_first(dynhash *);

#endif
