main()
{
	float a, b;

	a = 1.3f, b = 3.2f;

	printf("%f\n", a + b); // seems to be a stack alignment problem here

	return 0;
}
