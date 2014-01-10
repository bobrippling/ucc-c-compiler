struct A // not output - unused
{
	enum E
	{
		X123, Y123, Z123
	} e;
	int i, j;
};

static long f(int x)
{
	static int si = 3;

	si += x;

	x = 3;

	return si;
}

main(int argc, char *argv[])
{
	struct A
	{
		int i, j;
	} x = {
		f(52),
		3
	};

	for(; x.i > 0; x.i--){
		int add = x.i;
		x.j += f(add);
	}

	return x.i;
}
