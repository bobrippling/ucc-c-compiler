// RUN: %ocheck 3 %s
struct A
{
	int i, j;
};

main()
{
	struct A a = { 1, 2 }, b = a;

	return b.i + b.j;
}
