// RUN: %check %s

main()
{
	int restrict *p; // WARN: /restrict on non-pointer type/
}
