struct Tim
{
	int i;
	int j;
};

set_j(struct Tim *tp)
{
	tp->j = 5;
}

main()
{
	struct Tim t;

	set_j(&t);

	return t.j;
}
