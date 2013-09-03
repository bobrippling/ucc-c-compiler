f(float i, float j)
{
	float k = 7;

	printf("%f\n", i + j + k);
}

main()
{
	f(1, 2); // FIXME: should be using xmm0 and eax here, not xmm1/bx
}
