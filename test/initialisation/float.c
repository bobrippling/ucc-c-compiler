// RUN: %ocheck 8 %s
struct
{
	char c;
	float f;
	double d;
} a[] = {
	1, 2, 3,
	{ 4, 5 },
	6,
};

main()
{
	return a[1].d + a[2].c + a->f;
}
