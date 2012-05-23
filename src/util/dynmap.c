#include <stdlib.h>
#warning remove stdio include
#include <stdio.h>

#include "dynmap.h"
#include "alloc.h"

typedef struct pair pair;

struct dynmap
{
	struct pair
	{
		void *key, *value;
		pair *next;
	} *pairs;
};

dynmap *dynmap_new(void)
{
	dynmap *m = umalloc(sizeof *m);
	return m;
}

void dynmap_free(dynmap *map)
{
	pair *p, *q;

	for(p = map->pairs; p; q = p->next, free(p), p = q);

	free(map);
}

void *dynmap_get(dynmap *map, void *key)
{
	pair *i;

	for(i = map->pairs; i; i = i->next)
		if(i->key == key){
			fprintf(stderr, "dynmap_get %p key %p (%s) = %p\n", (void *)map, key, (char *)key, i->value);

			return i->value;
		}

	fprintf(stderr, "dynmap_get %p key %p (%s) = %p\n", (void *)map, key, (char *)key, NULL);

	return NULL;
}

void dynmap_set(dynmap *map, void *key, void *val)
{
	pair *p = dynmap_get(map, key);

	if(p){
		fprintf(stderr, "dynmap_set %p key %p (%s) val %p replace (%p)\n",
				(void *)map, key, (char *)key, val, p->value);
		p->value = val;
	}else{
		p = umalloc(sizeof *p);
		p->key   = key;
		p->value = val;
		p->next = map->pairs;
		map->pairs = p;

		fprintf(stderr, "dynmap_set %p key %p (%s) val %p new\n",
				(void *)map, key, (char *)key, val);
	}
}

void *dynmap_key(dynmap *map, int i)
{
	pair *p;

	for(p = map->pairs; p && i > 0; p++, i--);

	fprintf(stderr, "dynmap_key %p index %d %p (%s)\n",
			(void *)map, i, (void *)p, p ? (char *)p : "(n/a)");

	return p ? p->key : NULL;
}
