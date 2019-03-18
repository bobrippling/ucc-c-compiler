enum BM
{
	A = 1 << 3, B, C,
} __attribute__((bitmask));

P(enum BM i)
{
	printf("%d\n", i);
}

main()
{
	enum BM a = A | C;

	P(A);
	P(B);
	P(C);
}
