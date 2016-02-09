struct A
{
int a : 4;
int b : 2;
int c : 1;
int : 0;
short x : 3;
int z : 7;
};

f(struct A *p)
{
return p->c + p->x + p->z;
}

main()
{
	struct A a = { 0 };

	a.a = 2;
	a.b = 3;
	a.c = 0;
	a.x = 1;
	a.z = 7;

	printf("f(&{ %d, %d, %d, %d, %d }) = %d\n",
			a.a,
			a.b,
			a.c,
			a.x,
			a.z,
			f(&a));
}
