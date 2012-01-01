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

void dynarray_rm(void ***par, void *x)
{
	void **ar;
	int i, n;

	ar = *par;
	n = dynarray_count(ar);

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
		while(*ar){
			f(*ar);
			ar++;
		}
		free(*par);
		*par = NULL;
	}
}
