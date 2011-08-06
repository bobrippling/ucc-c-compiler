int write(int, int, int);

void printd_rec(int n)
{
	int d, m;
	d = n / 10;
	m = n % 10;
	if(d)
		printd(d);
	m = m + '0';
	write(1, &m, 1);
}

void printd(int n)
{
	int nl;

	if(n < 0){
		int neg;
		n = -n;
		neg = '-';
		write(1, &neg, 1);
	}

	printd_rec(n);
	nl = '\n';
	write(1, &nl, 1);
}
