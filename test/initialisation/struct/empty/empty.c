// RUN: %ucc -o %t %s
// RUN: %t | %output_check '0 0 0 8'

struct A
{
};

struct Containter
{
	struct A a;
};

struct Pre
{
	int i;
	struct A a;
	int j;
};

main()
{
	struct A a;
	struct Containter b;
	struct Pre c;

	printf("%d %d %d %d\n",
			sizeof(struct A),
			sizeof a,
			sizeof b,
			sizeof c);

	c.i = 1;
	c.j = 2;
}
