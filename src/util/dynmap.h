#ifndef DYNMAP_H
#define DYNMAP_H

typedef struct dynmap dynmap;
/* 0 for match, non-zero for mismatch */
typedef int dynmap_cmp_f(void *, void *);

dynmap *dynmap_new(dynmap_cmp_f);
void    dynmap_free(dynmap *);

void *dynmap_nochk_get(dynmap *, void *key);
void  dynmap_nochk_set(dynmap *, void *key, void *val);

void *dynmap_nochk_key(dynmap *map, int i);

void *dynmap_nochk_value(dynmap *map, int i);

#include "dyn.h"

/* TODO */
#warning TODO

#define dynmap_get(type_k, type_v, map, key)   \
	(DYN_CMP(type_k, key),                       \
	 (type_v)dynmap_nochk_get(map, (void *)key))

#endif
