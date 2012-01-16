#define BASIC

#ifdef BASIC
main()
{
	struct
	{
		int i, j;
		int *p;
	} *x, y;

	x = &y;

	x->i = 5;
	x->j = 2;

	return x->i;
}
#endif

#ifdef SIMPLE
main()
{
	struct
	{
		int a, b;
		struct
		{
			int d, e;
		} sub;
		int c;
	} *list, actual;

	list = &actual;

	list->a = 1;
	list->b = 2;
	list->c = 3;
	list->sub->d = 4;
	list->sub->e = 5;

	return list->sub->d + list->c;
}
#endif

#ifdef NESTED
main()
{
	struct int_struct_ptr
	{
		int i;
		struct ptr_struct
		{
			void *p;
			int i;
		} sub;
		void *p;
	} x;

	x.sub.i = 5;

	return x.sub.i;
}
#endif

#ifdef NAMING
struct timmy
{
	int i;
	void *p;
};
struct timmy tim; // FIXME

struct
{
	struct
	{
		int i;
	}; // TODO: empty decl or whatever gcc says
	int j;
} tim;
#endif
