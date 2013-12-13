// RUN: %ocheck 1 %s
struct F
{
	int n;
	char s[];
};

copy(struct F *to, struct F *from)
{
	*to = *from;
}

main()
{
	struct F a, b;

	a.n = 5;

	copy(&b, &a);

	return b.n == 5;
}
