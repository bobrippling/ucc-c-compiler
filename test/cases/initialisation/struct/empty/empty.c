// RUN: %ucc -o %t %s
// RUN: %t | grep '^0 0 0 8$'

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

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
