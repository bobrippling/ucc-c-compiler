// RUN: %ocheck 5 %s
struct A
{
	struct
	{
		int i;
	};
	int j;
} a;

main()
{
	struct A a;
	*(int *)&a = 5;
	return a.i;
}
