// RUN: %ucc -o %t %s && %t

unsigned long foo() {
	return (unsigned long) - 1 / 8;
	// correct interpretation is ((unsigned long) -1) / 8, a pretty large number
	// incorrect, i.e. (unsigned long)(-1 / 8), is zero
}

main()
{
	return foo() == 0 ? 1 : 0;
}
