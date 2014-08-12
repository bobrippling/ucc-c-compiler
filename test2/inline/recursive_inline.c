// RUN: %ucc -o %t %s
// RUN: %inline_check %s

f(int);

g(int i)
{
	return 2 + f(i); // this should fail to inline eventually
}

f(int i)
{
	return 1 + g(i);
}

main()
{
	return f(2);
}
