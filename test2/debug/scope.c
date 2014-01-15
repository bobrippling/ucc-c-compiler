// RUN: %ucc -g %s -o %t

struct A // not output - unused
{
	enum E
	{
		X123, Y123, Z123
	} e;
	int i, j;
};

struct B
{
	enum E2
	{
		X321, Y321, Z321
	} e;
	int i, j;
} b;

static int f(int i)
{
	static int si = 3;
	return i + si;
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
}
