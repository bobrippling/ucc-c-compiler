// RUN: %ucc %s
main()
{
	int x;
	return x == (1 && x);
}
