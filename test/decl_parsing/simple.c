// RUN: %ucc -o %t %s
// RUN: %ocheck 5 %t

void set_to_3(int *p)
{
	*p = 3;
}

void set_to_5(int **p)
{
	**p = 5;
}

main()
{
	int i;
	int *const p = &i;
	int **pp;

	pp = &p;
	*p = 5;
	set_to_3(p);
	set_to_5(pp);

	return i;
}
