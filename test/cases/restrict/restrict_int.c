// RUN: %check %s

main()
{
	int restrict a; // CHECK: /restrict on non-pointer type/
}
