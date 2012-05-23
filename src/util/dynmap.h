#ifndef DYNMAP_H
#define DYNMAP_H

typedef struct dynmap dynmap;

dynmap *dynmap_new(void);
void    dynmap_free(dynmap *);

void *dynmap_get(dynmap *, void *key);
void  dynmap_set(dynmap *, void *key, void *val);

void *dynmap_key(dynmap *, int);

#endif
