// RUN: %ocheck 8 %s
struct s
{
	int n;
	// 4 padding
	long l[];
};

main()
{
	return sizeof(struct s);
}
