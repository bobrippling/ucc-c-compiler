// RUN: %ocheck 1 %s

_Bool f(float nan)
{
	/* implicit cast does a float cmp to zero */
	return nan;
}

main()
{
	union
	{
		unsigned char bytes[4];
		float f;
	} nun = { 0, 0, 0xc0, 0x7f };

	return f(nun.f);
}
