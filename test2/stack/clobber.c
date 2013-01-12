// RUN: %ucc %s -o %t
// RUN: [ `%t | grep -c 753` -eq 3 ]

wn(int n)
{
	int d = n / 10;
	int c = n % 10 + '0';

	if(d)
		wn(d);

	write(1, &c, 1);
}

nl()
{
	write(1, "\n", 1);
}

wn3(a, b, c)
{
	wn(a);
	nl();
	wn(b);
	nl();
	wn(c);
	nl();
}

main()
{
	int x;

	int y, h;

	y = 150;
	h = 300;
	x = 30;

	wn3(
			(x * x) / 256 + 3 * y + h,
			(x * x) / 256 + 3 * y + h,
			(x * x) / 256 + 3 * y + h);
}
