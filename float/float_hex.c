main()
{
	union
	{
		float f;
		int i;
	} a = {
		.i = 0x3f800000 // 1.0f
	};

	printf("%f\n", a.f);

	a.f = 0x52.3p-3;

	printf("%f\n", a.f);
}
