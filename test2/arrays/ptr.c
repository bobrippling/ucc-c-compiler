// RUN: %ucc %s

f(int (*pa)[])
{
	return (*pa)[1];
}
