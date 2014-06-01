#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "alloc.h"
#include "dynarray.h"
#include "util.h"

void dynarray_nochk_add(void ***par, void *new)
{
	void **ar = *par;
	int idx = 0;

	UCC_ASSERT(new, "dynarray_nochk_add(): adding NULL");

	if(!ar){
		ar = umalloc(2 * sizeof *ar);
	}else{
		idx = dynarray_nochk_count(ar);
		ar = urealloc1(ar, (idx + 2) * sizeof *ar);
	}

	ar[idx] = new;
	ar[idx+1] = NULL;

	*par = ar;
}

void *dynarray_nochk_padinsert(
		void ***par, unsigned i, unsigned *pn, void *ins)
{
	void **ar = *par;
	unsigned n = *pn;

	if(i < n){
		/* already have one, replace */
		void **p = &ar[i],
				 *out = *p;

		*p = ins;

		return out == DYNARRAY_NULL ? NULL : out;
	}else{
		/* pad up to it */
		unsigned j;
		for(j = i - n; j > 0; j--){
			dynarray_add(par, DYNARRAY_NULL);
			n++;
		}

		/* add */
		dynarray_add(par, ins);
		n++;

		*pn = n;
		return NULL;
	}
}

void *dynarray_nochk_pop(void ***par)
{
	void **ar = *par;
	void *r;
	int n;

	n = dynarray_nochk_count(ar);
	UCC_ASSERT(n > 0, "dynarray_nochk_pop(): empty array");

	r = ar[n - 1];
	ar[n - 1] = NULL;

	if(n == 1){
		free(ar);
		*par = NULL;
	}

	return r;
}

void dynarray_nochk_prepend(void ***par, void *new)
{
	void **ar;
	int i;

	dynarray_nochk_add(par, new);

	ar = *par;

//#define SLOW
#ifdef SLOW
	for(i = dynarray_nochk_count(ar) - 2; i >= 0; i--)
		ar[i + 1] = ar[i];
#else
	i = dynarray_nochk_count(ar) - 1;
	if(i > 0)
		memmove(ar + 1, ar, i * sizeof *ar);
#endif

	ar[0] = new;
}

void dynarray_nochk_rm(void **ar, void *x)
{
	int i, n;

	n = dynarray_nochk_count(ar);

	UCC_ASSERT(n, "dynarray_nochk_rm(): empty array");

	for(i = 0; ar[i]; i++)
		if(ar[i] == x){
			memmove(ar + i, ar + i + 1, (n - i) * sizeof *ar);
			return;
		}
}

int dynarray_nochk_count(void **ar)
{
	int len = 0;

	if(!ar)
		return 0;

	while(ar[len])
		len++;

	return len;
}

void dynarray_nochk_free(void ***par, void (*f)(void *))
{
	void **ar = *par;

	if(ar){
		if(f)
			while(*ar){
				f(*ar);
				ar++;
			}
		free(*par);
		*par = NULL;
	}
}

void dynarray_nochk_add_array(void ***par, void **ar2)
{
	void **ar = *par;
	int n, n2, total;

	if(!ar2)
		return;

	if(!ar){
		n = dynarray_nochk_count(ar2);
		ar = umalloc((n + 1) * sizeof *ar);
		memcpy(ar, ar2, n * sizeof *ar2);
		ar[n] = NULL;
		*par = ar;
		return;
	}

	n  = dynarray_nochk_count(ar);
	n2 = dynarray_nochk_count(ar2);

	total = n + n2;

	ar = urealloc1(ar, (total + 1) * sizeof *ar);
	memcpy(ar + n, ar2, (n2 + 1) * sizeof *ar2);

	*par = ar;
}

void dynarray_nochk_add_tmparray(void ***par, void **ar2)
{
	dynarray_nochk_add_array(par, ar2);
	dynarray_nochk_free(&ar2, NULL);
	/* can't have ***par2 since it might not be an lvalue in the macro,
	 * e.g. dynarray_add_tmparray(&ar, f())
	 */
}
