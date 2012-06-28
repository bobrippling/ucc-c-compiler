#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "alloc.h"
#include "dynarray.h"
#include "util.h"

void dynarray_add(void ***par, void *new)
{
	void **ar = *par;
	int idx = 0;

	UCC_ASSERT(new, "dynarray_add(): adding NULL");

	if(!ar){
		ar = umalloc(2 * sizeof *ar);
	}else{
		idx = dynarray_count(ar);
		ar = urealloc(ar, (idx + 2) * sizeof *ar);
	}

	ar[idx] = new;
	ar[idx+1] = NULL;

	*par = ar;
}

void *dynarray_pop(void ***par)
{
	void **ar = *par;
	void *r;
	int i;

	i = dynarray_count(ar) - 1;
	r = ar[i];
	ar[i] = NULL;

	UCC_ASSERT(r, "dynarray_pop(): empty array");

	if(i == 0){
		free(ar);
		*par = NULL;
	}

	return r;
}

void dynarray_prepend(void ***par, void *new)
{
	void **ar;
	int i;

	dynarray_add(par, new);

	ar = *par;

//#define SLOW
#ifdef SLOW
	for(i = dynarray_count(ar) - 2; i >= 0; i--)
		ar[i + 1] = ar[i];
#else
	i = dynarray_count(ar) - 1;
	if(i > 0)
		memmove(ar + 1, ar, i * sizeof *ar);
#endif

	ar[0] = new;
}

void dynarray_rm(void **ar, void *x)
{
	int i, n;

	n = dynarray_count(ar);

	UCC_ASSERT(n, "dynarray_rm(): empty array");

	for(i = 0; ar[i]; i++)
		if(ar[i] == x){
			memmove(ar + i, ar + i + 1, (n - i) * sizeof *ar);
			return;
		}
}

int dynarray_count(void **ar)
{
	int len = 0;

	if(!ar)
		return 0;

	while(ar[len])
		len++;

	return len;
}

void dynarray_free(void ***par, void (*f)(void *))
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

void dynarray_add_array(void ***par, void **ar2)
{
	void **ar = *par;
	int n, n2, total;

	UCC_ASSERT(ar2, "dynarray_add_array(): empty array");

	if(!ar){
		n = dynarray_count(ar2);
		ar = umalloc((n + 1) * sizeof *ar);
		memcpy(ar, ar2, n * sizeof *ar2);
		ar[n] = NULL;
		*par = ar;
		return;
	}

	n  = dynarray_count(ar);
	n2 = dynarray_count(ar2);

	total = n + n2;

	ar = urealloc(ar, (total + 1) * sizeof *ar);
	memcpy(ar + n, ar2, (n2 + 1) * sizeof *ar2);

	*par = ar;
}
