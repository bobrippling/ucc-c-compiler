// RUN: %ocheck 16 %s
struct A
{
	char x;
	double ar[];
};

main()
{
	return _Alignof(struct A) + sizeof(struct A);
}
