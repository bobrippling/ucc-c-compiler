#ifndef DYNARRAY_H
#define DYNARRAY_H

void  dynarray_add(    void ***, void *);
void  dynarray_prepend(void ***, void *);
void *dynarray_pop(    void **);
void  dynarray_rm(     void **,  void *);
int   dynarray_count(  void **);
void  dynarray_free(   void ***par, void (*f)(void *));


#endif
