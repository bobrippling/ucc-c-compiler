// RUN: ! %ucc %s

f(int *x)
{
	return 2 - x;
}
