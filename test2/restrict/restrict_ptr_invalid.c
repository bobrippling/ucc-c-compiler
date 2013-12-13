// RUN: %check %s

main()
{
	int restrict *p; // CHECK: /restrict on non-pointer type/
}
