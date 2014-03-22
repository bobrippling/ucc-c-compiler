// RUN: %check -e %s

main()
{
	const __auto_type k = 2;
	k = 3; // CHECK: error:
}
