struct A
{
	int a : 4;
	int c : 1;
	int : 0;
	short x : 3;
	int z : 7;
};

int f(struct A *p)
{
	return p->c + p->x + p->z;
}

void init(struct A *p)
{
	p->a = 2;
	p->c = 0;
	p->x = 1;
	p->z = 7;
}

void print(struct A *p)
{
	printf("f(&{ %d, %d, ",
			p->a,
			p->c);

	printf("%d, %d }) = %d\n",
				p->x,
				p->z,
				f(p));
}

main()
{
	struct A a; // = { 0 };

	init(&a);

	print(&a);
}

/*
expected:

type $struct1_A = {i4, i2}
type $struct2_B = {i4, i4}
type $struct3_C = {i4, i4}
type $struct4_D = {i4, i4}
*/
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
