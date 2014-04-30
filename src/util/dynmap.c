#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "dynmap.h"
#include "alloc.h"
#include "compiler.h"

#define HASH_TBL_CNT 256

typedef struct pair pair;

struct dynmap
{
	dynmap_cmp_f *cmp;
	dynmap_hash_f *hash;
	struct pair
	{
		void *key, *value;
		pair *next;
	} pairs[HASH_TBL_CNT];
};

unsigned dynmap_strhash(const void *v)
{
	const char *s = v;
	unsigned hash = 5381;

	for(; *s; s++)
		hash = ((hash << 5) + hash) + *s;

	return hash;
}

dynmap *
dynmap_new(dynmap_cmp_f cmp, dynmap_hash_f hash)
{
	dynmap *m = umalloc(sizeof *m);
	m->cmp = cmp;
	m->hash = hash;
	return m;
}

void
dynmap_free(dynmap *map)
{
	free(map);
}

static pair *
dynmap_nochk_pair(dynmap *map, void *key, unsigned *phash)
{
	pair *p;
	unsigned hash;

	assert(key && "null key");

	hash = map->hash(key);
	if(phash)
		*phash = hash;

	for(p = &map->pairs[hash % HASH_TBL_CNT];
	    p && p->key;
	    p = p->next)
	{
		if(map->cmp ? !map->cmp(p->key, key) : p->key == key)
			return p;
	}

	return NULL;
}

void *
dynmap_nochk_get(dynmap *map, void *key)
{
	pair *i;

	if(!map)
		return NULL;

	i = dynmap_nochk_pair(map, key, NULL);
	if(i)
		return i->value;

	return NULL;
}

int dynmap_nochk_exists(dynmap *map, void *key)
{
	if(!map)
		return 0;

	return !!dynmap_nochk_pair(map, key, NULL);
}

void *
dynmap_nochk_set(dynmap *map, void *key, void *val)
{
	pair *p;
	unsigned hash;

	assert(key && "null dynmap key");

	p = dynmap_nochk_pair(map, key, &hash);

	if(p){
		void *old = p->value;
		p->value = val;
		return old;
	}else{
		p = &map->pairs[hash % HASH_TBL_CNT];
		if(p->key){
			while(p->next)
				p = p->next;

			p->next = umalloc(sizeof *p);
			p = p->next;
		}

		p->key   = key;
		p->value = val;

		return NULL; /* no old value */
	}
}

static pair *
dynmap_nochk_idx(dynmap *map, int at)
{
	int i;

	if(!map)
		return NULL;

	for(i = 0; i < HASH_TBL_CNT; i++){
		pair *p;

		for(p = &map->pairs[i]; p; p = p->next){
			if(p->key){
				if(at == 0)
					return p;
				at--;
			}
		}
	}

	return NULL;
}

void *
dynmap_nochk_key(dynmap *map, int i)
{
	pair *p = dynmap_nochk_idx(map, i);
	return p ? p->key : NULL;
}

void *
dynmap_nochk_value(dynmap *map, int i)
{
	pair *p = dynmap_nochk_idx(map, i);
	return p ? p->value : NULL;
}

void *dynmap_nochk_rm(dynmap *map, void *key)
{
	unsigned hash;
	pair *p = dynmap_nochk_pair(map, key, &hash);
	void *value;

	if(!p)
		return NULL;

	value = p->value;

	if(p == &map->pairs[hash % HASH_TBL_CNT]){
		/* replace */
		pair *ent = &map->pairs[hash % HASH_TBL_CNT];

		if(p->next)
			memcpy_safe(ent, p->next);
		else
			memset(ent, 0, sizeof *ent);

	}else{
		pair *i;

		/* find the one before */
		for(i = &map->pairs[hash % HASH_TBL_CNT];
		    i->next != p;
		    i = i->next)
			;

		/* unlink */
		i->next = p->next;
		free(p);
	}

	return value;
}
