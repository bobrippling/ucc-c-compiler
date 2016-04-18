// RUN: %ucc -c %s

void func(int, int, int, int, int, int, int, int);


main()
{
	int a, b, c, d, e, f, g, h;
	a = b = c = d = e = f = g = h = 1;
	func(a, b, c, d, e, f, g, h);
}
