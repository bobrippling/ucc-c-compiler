// RUN: %check %s

main()
{
	// 'a' is used
a: &&a; // CHECK: !/warning/
}
