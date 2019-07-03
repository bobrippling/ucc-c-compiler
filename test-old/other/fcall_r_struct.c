struct Tim { int i, j; } *p()
{
	__typeof(*p()) *r = malloc(sizeof *r);
	r->i = 5;
	r->j = 2;
	return r;
}

main()
{
	if(p()->j != 2)
		abort();
	return 0;
}
