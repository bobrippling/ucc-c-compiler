// RUN: %ocheck 4 %s
typedef int flex[];

struct A
{
	int n;
	flex ar;
};

main()
{
	return sizeof(struct A);
}
