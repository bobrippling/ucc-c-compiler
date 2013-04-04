// RUN: %check %s

main()
{
	int restrict a; // WARN: /restrict on non-pointer type/
}
