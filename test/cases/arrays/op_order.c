// RUN: %ucc %s -o %t
// RUN: %t

f(char *x)
{
	return *(2 + x);
}

main()
{
	return f("abc") == 'c' ? 0 : 1;
}
