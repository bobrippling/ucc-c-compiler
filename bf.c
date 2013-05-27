struct half_bytes
{
	int a : 4;
	int b : 4;
};

main()
{
	struct half_bytes a;

	a.a = 5;
	a.b = 7;

	// *(char *)a
	// = 2 + 4 << 4
	// = 2 + (0b100 << 4)
	// = 2 + (0b1000000 << 4)
	// = 2 + 64
	// = 66 ???

	printf("%d %d\n", sizeof a, *(unsigned char *)&a);
}
