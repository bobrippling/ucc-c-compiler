// RUN: %ocheck 0 %s

struct A
{
};

struct A f()
{
	return (struct A){ 1, 2 };
}

main()
{
	struct A a = f();

	return sizeof a;
}
