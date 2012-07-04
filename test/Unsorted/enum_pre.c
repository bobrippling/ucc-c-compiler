enum X;

main()
{
	enum X a = 0;

	a = HI; // error

	return a;
}

enum X
{
	HI = 1
};
