nl()
{
		write(1, "\n", 1);
}

void var(int a, ...)
{
	int *l;

	l = &a + 1;

	while(a){
		//pf(1, a);
		//putchar(a + '0');
		int tmp = a + '0';
		write(1, &tmp, 1);
		nl();
		a = *l;
		l++;
	}
}

void main()
{
	var(5, 4, 3, 2, 1, 0);
}
