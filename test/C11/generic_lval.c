// RUN: %ocheck 0 %s

// test address for static storage duration - const-fold checks
int i;
int *p = &_Generic(i, int: i);

int f(int *);
void v(void);

f(int *p)
{
	*p = 2;
}

main()
{
	// test address for auto storage duration - f_lea() check
	int j = 10;
	_Generic(0, int: f)(&_Generic(5, int: j));

	if(j != 2)
		abort();

	_Static_assert(
			__builtin_types_compatible_p(
				__typeof(_Generic(0, int: v())),
				void),
			"void not propagated");

	if(p != &i)
		abort();

	return 0;
}
