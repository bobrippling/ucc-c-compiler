// RUN: %ocheck 0 %s

struct A
{
	int x[5];
};

main()
{
	(struct A){
		{ 1, 2, 3, 4, 5 },
	};

	struct A x = { 0 };

	(void)x;

	return 0;
}
