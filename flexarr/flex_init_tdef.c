typedef int flex[];

struct A
{
	int n;
	flex ar;
};

main()
{
	struct A x = { 5, { 1, 2, 3 } };
	return sizeof(x);
}
