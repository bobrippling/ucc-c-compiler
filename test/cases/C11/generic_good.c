// RUN: %ocheck 0 %s

extern _Noreturn void abort(void);

int abort2()
{
	abort();
}

void notcalled()
{
	const int f();
	int g();

	_Static_assert(_Generic(f(), const int: 0, int: 1) == 1);
	_Static_assert(_Generic(g(), const int: 0, int: 1) == 1);
}

void array_len()
{
#define DOUBLE_LENGTH(A) (\
		sizeof _Generic( \
		  (A), \
		  double*: (A)[0], \
		  default: _Generic( \
		             &(A)[0], \
		             double*: (A)))) \
		   / sizeof(double)

	double a[3];
	double *p;

	_Static_assert(DOUBLE_LENGTH(a) == 3);
	_Static_assert(DOUBLE_LENGTH(&(double){3}) == 1);
	_Static_assert(DOUBLE_LENGTH(p) == 1);
}

main()
{
#include "../ocheck-init.c"
	if(_Generic(abort2(), int: 0))
		abort();

	if(_Generic(0, void *: abort2(), int: 3) != 3)
		abort();

	int x = 2;
	if(_Generic(0, int: x = 3) != 3)
		abort();
	if(x != 3)
		abort();

	const char k = 0; /* test on char - no decay/promotion */
	if(_Generic(k, char: 0, default: abort2()))
		abort();
	if(_Generic((const char)0, char: 0)) // XXX
		abort();

	int ar[1];
	const int kar[1];
	if(_Generic(ar, int [1]: 0, const int *: abort2())) // XXX
		abort();
	if(_Generic(kar, const int [1]: 0, int *: abort2())) // XXX
		abort();

#define DECAY(x) (0, x)

	if(_Generic(DECAY(main), int (*)(): 0)) // XXX
		abort();

	return 0;
}
