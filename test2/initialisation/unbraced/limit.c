// RUN: %ocheck 0 %s

sideeffect()
{
	abort();
}

a()
{
	int (*p)[2] = (int[][2]){ 1, 2, { 3, 4, sideeffect() } };
	// should be: { { 1, 2 }, { 3, 4 } } - sideeffect() ignored

	if(p[0][0] != 1
	|| p[0][1] != 2
	|| p[1][0] != 3
	|| p[1][1] != 4)
		abort();
}

struct A
{
	int i, j;
};

b()
{
	struct A *p = (struct A []){
		1, 2,
		3, 4
	};

	if(p->i != 1 || p->j != 2
	|| p[1].i != 3 || p[1].j != 4)
		abort();
}

c()
{
	struct A x[] = {
		1, { 2 }, 3
	};
	/* { 1, 2 }, { 3, 0 } */
	if(x[0].i != 1 || x[0].j != 2)
		abort();
	if(x[1].i != 3 || x[1].j != 0)
		abort();
}

main()
{
	a();
	b();
	b();
}
