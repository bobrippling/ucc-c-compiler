// RUN: %ucc -o %t %s
// RUN: %ocheck 5 %t
f(i);

f(i)
	int i;
{
	return i;
}

g(i, j)
	int *j;
{
	return *j + i;
}

main()
{
	return f(5);
}
