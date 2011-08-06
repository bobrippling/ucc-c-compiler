int write(int, int, int);

void printd_rec(int i)
{
	int d, m;
	d = i / 10;
	m = i % 10;
	if(d)
		printd(d);
	m = m + '0';
	write(1, &m, 1);
}

void printd(int i)
{
	int nl;
	printd_rec(i);
	nl = '\n';
	write(1, &nl, 1);
}
