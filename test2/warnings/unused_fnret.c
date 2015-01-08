// RUN: %check %s -Wunused-function-return

f()
{
	return 3;
}

main()
{
	f(); // CHECK: warning: unused expression (funcall)
}
