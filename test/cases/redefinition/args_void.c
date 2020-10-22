// RUN: %check --only %s

any(); // any args, can't determine
one();
any2();


// any args, can determine - ang2(1) is valid, but the arg is obv. unused
any2()
{
	return 0;
}

one(i) // CHECK: warning: old-style function declaration
	int i;
{
	return i;
}

main()
{
	any(5);
	one(1);
	any2(2); // CHECK: /warning: too many arguments to implicitly \(void\)-function/
	one(1, 2); // CHECK: /warning: too many arguments/
}
