#ifndef DYNSET_H
#define DYNSET_H

#error dynhash incomplete

void dynset_add(void ***, void *, int (*cmp)(const void *, const void *));
#define DYNSET_ADD(a, b, c)                     \
	dynset_add(                                   \
			(void ***)(a),                            \
			(b),                                      \
			(int (*)(const void *, const void *))(c))

#endif
