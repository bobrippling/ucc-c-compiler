// RUN: %ucc %s

f(int (*p)())
{
	int (*f[2])();

	f[0] = p;
}

main;
