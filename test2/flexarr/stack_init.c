// RUN: %check -e %s
struct A
{
	int n;
	int x[];
};

main()
{
	struct A bad = {
		1, 2 // CHECK: /error: non-static initialisation of flexible array/
	};
}

