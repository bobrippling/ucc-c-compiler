typedef int x;
typedef int *intptr_t;
typedef void vd;

vd x(intptr_t);

x main()
{
	x i;
	intptr_t q;
	x *p;
	intptr_t *pq;

	pq = &q;

	p = q = (x *)&i;
	(vd)(i = (x)2);
}
