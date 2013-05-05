// RUN: %ucc -o %t %s
// RUN: %ocheck 5 %t

struct A
{
	struct B
	{
		int i, j;
	} b;
} a = {
	.b.j = 5
};

main()
{
	return a.b.i + a.b.j;
}
