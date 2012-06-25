typedef struct { int d; } D;

struct composite
{
	int a;
	struct
	{
		int b;
		int c;
	};
	D; // only with -fms-extensions
	struct E // similarly
	{
		int e;
	};
};

main()
{
}
