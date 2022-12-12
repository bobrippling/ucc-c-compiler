// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

syntax(int n)
{
	int ar[n];
	int (*p)[n] = &ar;

	syntax(p);
}

assert(_Bool b)
{
	if(!b)
		abort();
}

f(int n)
{
#define NULL (void*)0
	short (*p)[n] = NULL;
	assert(p == NULL);

	short (*q)[n] = (void *)3;
	assert(q == (void *)3);

	__auto_type a = (int)(p + 1);
	__auto_type b = 3 * sizeof(short);
	if(a != b)
		abort();
}

main()
{
#include "../ocheck-init.c"
	f(3);

	return 0;
}
