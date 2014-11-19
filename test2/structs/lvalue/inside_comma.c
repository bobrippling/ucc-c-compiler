// RUN: %ocheck 3 %s

struct A
{
	int x;
};

main()
{
	struct A a = { 3 };

	return (0, a).x;
}
