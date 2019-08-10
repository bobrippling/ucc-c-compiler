// RUN: %ocheck 0 %s

main()
{
	float a, b;

	a = 1.3f, b = 3.2f;

	float q = a + b;
	if(q != 4.5)
		return 1;
	return 0;
}
