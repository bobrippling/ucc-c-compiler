#ifndef DYNMAP_H
#define DYNMAP_H

typedef struct dynmap dynmap;
typedef int dynmap_cmp_f(void *, void *);

dynmap *dynmap_new(dynmap_cmp_f);
void    dynmap_free(dynmap *);

void *dynmap_get(dynmap *, void *key);
void  dynmap_set(dynmap *, void *key, void *val);

void *dynmap_key(dynmap *map, int i);

void *dynmap_value(dynmap *map, int i);

#endif
