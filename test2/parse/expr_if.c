// RUN: %ucc -o %t %s && (%t; [ $? -eq 3 ])

long f(int i)
{
	// force a cast of the implicit missing expr `i' to long by code in expr_if
	// type prop.
	return i ? : (long)2;
}

main()
{
	0 ?: f(1);

	return f(3);
}
