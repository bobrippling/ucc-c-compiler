any(); // any args, can't determine
one();
any2();


any2() // any args, can determine - ang2(1) is valid, but the arg is obv. unused
{
}

one(i)
	int i;
{
	return i;
}

main()
{
	return any2(2);
}
