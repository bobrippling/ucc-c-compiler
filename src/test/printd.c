int write(int, int, int);

void printd(int i)
{
	int d, m;
	d = i / 10;
	m = i % 10;
	if(d)
		printd(d);
	write(1, m + 'a', 1);
}
