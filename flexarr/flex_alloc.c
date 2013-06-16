struct s
{
	int n;
	// 4 padding
	long l[];
};

main()
{
	f(sizeof(struct s) + sizeof(long[2]));
}
