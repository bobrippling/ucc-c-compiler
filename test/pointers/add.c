// RUN: %ocheck 6 %s

add(int a, int b)
{
	return (int)&a[(char *)b];
}

main()
{
	return add(1, 5);
}
