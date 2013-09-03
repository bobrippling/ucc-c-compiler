// RUN: %ocheck 1 %s

fadd(float a, float b, float c, float d)
{
	return a + b + c + d;
}

main()
{
	return 10 == (int)fadd(1, 2, 3, 4);
}
