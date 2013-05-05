// RUN: %ucc %s

main()
{
	return ((1 - sizeof(int)) >> 32);
}
