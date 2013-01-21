// RUN: %ucc %s -c
// RUN: [ `%ucc %s -c 2>&1 | wc -l` -eq 0 ]

int *restrict f8(void)
{
	extern int i, *p, *q = (void *)0, *r;

	r = (int * restrict)q;

	for(i = 0; i < 100; i++)
		*(int * restrict)p++ = r[i];

	return p;
}
