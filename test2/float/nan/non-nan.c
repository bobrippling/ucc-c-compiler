// RUN: %ocheck 0 %s

_Bool f(float f)
{
	return f;
}

main()
{
	return f(0);
}
