#ifdef BASIC
int main()
{
	struct
	{
		struct
		{
			int i, j;
		} st;
		int q;
	} *x, y;

	x = &y;

	x->st.i = 5;
	x->st.j = 2;
	y.q = 5;
	x->q = 1;

	y.st.i = 3;

	return x->st.i + x->st.j + y.q - 6;
}
#endif

#ifdef SIMPLE
int main()
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
	list->sub.d = 4;
	list->sub.e = 5;

	return list->sub.d + list->c - 7;
}
#endif

#ifdef EXPR_TEST
#include <assert.h>

struct A
{
	int i;
	int j;
};

void set_j_3(struct A *p)
{
	p->j = 3;
}

int main()
{
	struct A a, *p;
	void *vp;
	int val;

	vp = p = &a;

	p->j = 99;
	set_j_3(p);

	((struct A *)vp)->i = 2;

	val = p->i + p->j;
	assert(val == 5);
	return 0;
}
#endif

#ifdef NESTED
int main()
{
	struct int_struct_ptr
	{
		int i;
		struct ptr_int
		{
			void *p;
			int i;
		} sub;
		void *p;
	} x;

	x.sub.i = 5;

	return x.sub.i - 5;
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
} jim;

main()
{
	return 0;
}
#endif

#ifdef NESTED_DIFFERENT
int main()
{
	struct C
	{
		int a;
	};

	struct B
	{
		int a;
		struct C c;
	} st_b ;//= { 0, 0 };

	struct A
	{
		int a;
		struct B *b;
	} st_a;

	st_b.c.a = 3;

	st_a.b = &st_b;

	return st_a.b->c.a - 3;
}
#endif
