// RUN: %ocheck 4 %s
typedef int flex[];

struct A
{
	short n;
	// pad of 2
	flex ar;
};

main()
{
	static struct A x = { 5, { 1, 2, 3 } };
	return sizeof x;
}
