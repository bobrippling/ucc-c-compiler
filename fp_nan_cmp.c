#ifdef __TINYC__
#  define NAN __nan__
#else
#  define NAN __builtin_nanf("")
#endif

tst(float a, float b)
{
	// returns false for nans
	//return a > b || a >= b || a < b || a <= b;
	return a < b;
}

main()
{
	if(tst(NAN, 5))
		return 2;

	if(!tst(5, 6))
		return 1;

	if(tst(NAN, NAN))
		return 3;

	if(tst(5, NAN))
		return 4;

	return 0;
}
