// RUN: %check %s

main()
{
	// 'a' is used
a: // CHECK: !/warning/
	&&a;
}
