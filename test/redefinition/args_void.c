// RUN: %check %s

any(); // any args, can't determine
one();
any2();


// any args, can determine - ang2(1) is valid, but the arg is obv. unused
any2()
{
	return 0;
}

one(i)
	int i;
{
	return i;
}

main()
{
	any(5);  // CHECK: !/warn/
	one(1);  // CHECK: !/warn/
	any2(2); // CHECK: /warning: too many arguments to implicitly \(void\)-function/
	one(1, 2); // CHECK: /warning: too many arguments/
}
