struct Tim{int i,j;}*p()
{
	__typeof(*p()) *r = malloc(sizeof *r);
	r->i = 5;
	r->j = 2;
	return r;
}

main()
{
	printf("%d y halo thar %d %s\n", p()->j, 5, "yo yo");
}
