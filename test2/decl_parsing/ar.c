// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 4 ]
plus(i)
{
	return i + 1;
}

minus(i)
{
	return i - 1;
}

main()
{
	int (*f[2])();

	f[0] = plus;
	f[1] = minus;

	return f[1](5);
}
