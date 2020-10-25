// RUN: %ocheck 0 %s -fno-integral-float-load

f(int p)
{
	// the following expression caused a bug in -Os,
	// where out_new_zero() would return a float out_val,
	// in the normalise() code
	//
	// seeing a V_CONST_F would cause the backend to try
	// to load a 0 from a (float) label, running into type
	// problems, since the out_val was { .type = V_CONST_F, .t = type_int }

	return p && 1;
}

main()
{
	void abort();

	if(f(0) != 0) abort();
	if(f(1) != 1) abort();
	if(f(2) != 1) abort();

	return 0;
}
