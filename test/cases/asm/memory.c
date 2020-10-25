// RUN: %ucc -c %s

int glob;

f_a()
{
	int local;

	local = 2;

	glob = local;
}

f_b()
{
	*(short *)5 = 3;

}

f_c()
{
	int *p;

	p = &glob;

	*p = 7;
}

f_d()
{
	int a;
	int *p;

	p = &a;

	*p = 3;
}
