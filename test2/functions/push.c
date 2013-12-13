// RUN: %ucc -c %s

main()
{
	int a, b, c, d, e, f, g, h;
	a = b = c = d = e = f = g = h = 1;
	func(a, b, c, d, e, f, g, h);
}
