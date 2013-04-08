// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 3 ]

f() __attribute(());

f(i)
	__attribute__(()) int i;
{
	return i + 1;
}

main()
{
	return f(2);
}
