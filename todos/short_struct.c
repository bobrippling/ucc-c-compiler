struct shorts
{
	short s1, s2;
};

main()
{
	struct shorts shrts;
	shrts.s1 = 5;
	shrts.s2 = 3;
	return shrts.s1 + shrts.s2;
}
