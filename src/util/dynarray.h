#ifndef DYNARRAY_H
#define DYNARRAY_H

void  dynarray_nochk_add(    void ***, void *);
void  dynarray_nochk_prepend(void ***, void *);
char *dynarray_nochk_pop(    void ***);
void  dynarray_nochk_rm(     void **,  void *);
int   dynarray_nochk_count(  void **);
void  dynarray_nochk_free(   void ***par, void (*f)(void *));
void  dynarray_nochk_add_array(void ***, void **);

#define DYNARRAY_NULL (void *)1

#include "dyn.h"

#define DYNARRAY_CHECK(ar, arg, func, ...) \
	(UCC_TYPECHECK(__typeof(arg) **, ar),    \
	func(__VA_ARGS__))

#define dynarray_add(ar, p)     DYNARRAY_CHECK(ar, p, dynarray_nochk_add,     (void ***)(ar), (void *)(p))
#define dynarray_prepend(ar, p) DYNARRAY_CHECK(ar, p, dynarray_nochk_prepend, (void ***)(ar), (void *)(p))

#define dynarray_pop(t, ar)             \
	(UCC_TYPECHECK(t **, ar),             \
	 (t)dynarray_nochk_pop((void ***)ar))


#define dynarray_rm(ar, p)            \
	(UCC_TYPECHECK(__typeof(p) *, ar),  \
	dynarray_nochk_rm((void **)ar, p))


#define dynarray_free(ar, fn)            \
	 dynarray_nochk_free((void ***)ar, fn)


#define dynarray_add_array(ar, sub)                      \
	(UCC_TYPECHECK(__typeof(sub) *, ar),                   \
	 dynarray_nochk_add_array((void ***)ar, (void **)sub))


#define dynarray_count(ar) dynarray_nochk_count((void **)ar)

#endif
