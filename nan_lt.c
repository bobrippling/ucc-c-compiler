gt(float a, float b)
{
	return a < b;
}

main()
{
	return gt(__builtin_nanf(""), 1);
}
