#ifndef DYNARRAY_H
#define DYNARRAY_H

void  dynarray_nochk_add(    void ***, void *);
void  dynarray_nochk_prepend(void ***, void *);
void *dynarray_nochk_pop(    void ***);
void  dynarray_nochk_rm(     void ***,  void *);
int   dynarray_nochk_count(  void **);
void  dynarray_nochk_free(   void ***par, void (*f)(void *));
void  dynarray_nochk_add_array(void ***, void **);
void  dynarray_nochk_add_tmparray(void ***, void **);

void *dynarray_nochk_padinsert(void ***par,
		unsigned i, unsigned *pn, void *ins);

void dynarray_nochk_insert(
		void ***par, unsigned i, void *to_insert);

#define DYNARRAY_NULL (void *)1

#include "dyn.h"

#define DYNARRAY_CHECK(ar, arg, func, ...) \
	(UCC_TYPECHECK(__typeof((void)0, arg) **, ar),    \
	func(__VA_ARGS__))

#define dynarray_add(ar, p)     DYNARRAY_CHECK(ar, p, dynarray_nochk_add,     (void ***)(ar), (void *)(p))
#define dynarray_prepend(ar, p) DYNARRAY_CHECK(ar, p, dynarray_nochk_prepend, (void ***)(ar), (void *)(p))

#define dynarray_padinsert(ar, i, n, p) \
	DYNARRAY_CHECK(ar, p,                 \
			dynarray_nochk_padinsert,         \
			(void ***)(ar), i, n, (void *)(p))

#define dynarray_insert(ar, i, p) \
	DYNARRAY_CHECK(ar, p,           \
			dynarray_nochk_insert,      \
			(void ***)(ar), i, (void *)(p))

#define dynarray_pop(t, ar)             \
	(UCC_TYPECHECK(t **, ar),             \
	 (t)dynarray_nochk_pop((void ***)ar))


#define dynarray_rm(ar, p)            \
	(UCC_TYPECHECK(__typeof(p) **, ar), \
	dynarray_nochk_rm((void ***)ar, p))

#define dynarray_free(ty, ar, fn)            \
	(UCC_TYPECHECK(ty, ar),                    \
	 dynarray_nochk_free((void ***)&(ar), fn))

#define dynarray_add_array(ar, sub)                      \
	(UCC_TYPECHECK(__typeof(sub) *, ar),                   \
	 dynarray_nochk_add_array((void ***)ar, (void **)sub))

#define dynarray_add_tmparray(ar, tsub) \
	(UCC_TYPECHECK(__typeof(tsub) *, ar), \
	 dynarray_nochk_add_tmparray((void ***)ar, (void **)tsub))

#define dynarray_count(ar) dynarray_nochk_count((void **)ar)

#endif
