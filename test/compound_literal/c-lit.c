// RUN: %ocheck 5 %s

f()
{
	int i = (int){2};
	return i;
}

main()
{
	return (int[]){1, 2, 3, 4}[2] + f();
}
