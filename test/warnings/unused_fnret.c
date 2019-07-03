// RUN: %check %s -Wunused-return-value

f()
{
	return 3;
}

main()
{
	f(); // CHECK: warning: unused expression (function-call)
}
