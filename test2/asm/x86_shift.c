// RUN: %ucc %s
main()
{
	short s;
	long l;
	char c;

	return (l << c) + (s << l);
}
