// RUN: %check %s

f(char restrict *p); // CHECK: warning: restrict on non-pointer type 'char restrict'

int *restrict f8(void)
{
	extern int i, *p, *r; // CHECK: !/warn/
	int *q = (void *)0; // CHECK: !/warn/

	r = (int * restrict)q; // CHECK: !/warn/

	for(i = 0; i < 100; i++) // CHECK: !/warn/
		*(int * restrict)p++ = r[i]; // CHECK: !/warn/

	return p; // CHECK: !/warn/
}
