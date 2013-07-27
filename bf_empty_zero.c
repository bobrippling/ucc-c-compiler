/*
struct A
{
	unsigned *pad* : 1;
	unsigned i : 1;
};

main()
{
	struct A x;

	x.i = 1;
}
*/

struct A
{
	int : 0;
};

struct A a;
struct A b = {};
struct A c = { 1 };
struct A *d = &c;
struct A e[] = { {}, {}, {} };

main()
{
	struct A x[10];

	f(&x[2], &x[5]);

	return sizeof(x);
}
