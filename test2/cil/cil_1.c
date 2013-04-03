// RUN: %ucc %s
main()
{
	return -3 >> (8 * sizeof(int));
}
