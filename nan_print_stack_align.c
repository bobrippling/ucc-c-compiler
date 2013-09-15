f(float f)
{
	printf("%f\n", f);
}

main()
{
	f(__builtin_nanf(""));
}
