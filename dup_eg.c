typedef struct
{
	int *a, *b;
} r;

/*
f(r *p)
{
	++*p->b;
}
*/

main()
{
	int i = 5;

	r r = { .b = &i };
	f(&r);
	//f(&(r){ .b = &i });

	return i;
}
