#define STRUCT_PTR

#ifdef PRE_DECL_STRUCT
struct timmy
{
	int i;
	void *p;
};

main()
{
	struct timmy tim; // FIXME
	1;
}
#endif

#ifdef STRUCT_PTR
main()
{
	int array_hack[64];
	struct int_struct_ptr
	{
		int i;
		struct ptr_struct
		{
			int *p;
		} sub;
		void *p;
	} y, *x;

	x = array_hack;
	x->i = 5;

	y->i = 2;

	return x->i;
}
#endif

#ifdef TEST_STRUCT_EXPR
main()
{
	struct
	{
		int i;
	} *tim;
	int *p;

	p = &tim->i;
}
#endif

#ifdef TEST_ALLOC
main()
{
	struct { int i; int *p; } *st;

	st = malloc(sizeof *st);

	st->p = &st->i;
	*st->p = 5;

	return st->i;
}
#endif


#ifdef TEST_DOT
main()
{
	int q;
	x.sub.p = &x.i;
	*x.sub.p = 5;

	return x.i;
}
#endif
