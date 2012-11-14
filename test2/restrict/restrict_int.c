// RUN: %ucc %s -S 2>&1 | %check %s

main()
{
	int restrict a; // WARN: /restrict on non-pointer type/
}
