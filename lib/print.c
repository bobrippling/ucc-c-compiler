int write(int, char *, int);

printd_rec(int n)
{
	int d, m;
	d = n / 10;
	m = n % 10;
	if(d)
		printd_rec(d);
	m = m + '0';
	write(1, &m, 1);
}

printd(int n)
{
	if(n < 0){
		int neg;
		n = -n;
		neg = '-';
		write(1, &neg, 1);
	}

	printd_rec(n);
}

printc(char c)
{
	write(1, &c, 1);
}

printstr(char *str)
{
	/*
	if(!str)
		printstr("(null)");
	else
	*/
		while(*str){
			printc(*str);
			str = str + 1;
		}
}
