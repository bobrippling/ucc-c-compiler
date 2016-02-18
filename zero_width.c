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

struct D
{
	int a : 3;
	int : 0;
	int b;
};

/*
type $struct1_A = {i4, i2}
type $struct2_B = {i4, i4, i4}
type $struct3_C = {i4, i4, i4}
type $struct4_D = {i4, i4}
*/

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
