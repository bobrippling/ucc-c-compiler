// RUN: %ocheck 3 %s

int x;

clean_any(void *p)
{
	x++;
}
#define cleanup __attribute__((cleanup(clean_any)))

main()
{

	{
		cleanup struct A { int i, j, k; } a; // CHECK: !/warn/
		cleanup int x; // CHECK: !/warn/
		cleanup char **p; // CHECK: !/warn/

		(void)a;
		(void)x;
		(void)p;
	}

	return x;
}
