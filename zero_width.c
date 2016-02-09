struct A
{
	int a : 4;
	int b : 2;
	int c : 1;
	int : 0;
	short x : 3;
	int z : 7;
};

struct B
{
	int a;
	int : 0;
	int b;
};

struct C
{
	int a;
	int : 0;
	int b : 3;
};

/*
f(struct A *p)
{
	g(p->a);
	g(p->b);
	g(p->c);

	g(p->x);
	g(p->z);
}
h(struct C *);
g(struct B *);
*/
