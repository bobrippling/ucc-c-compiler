// RUN: %ocheck 19 %s

f(float *p, float v)
{
	int r = (int)v + (int)*p;
	*p = 7;
	return r;
}

main()
{
	float v, w;

	w = v = 5;

	v = f(&w, v); // v=10, w=7

	v = (int)v + 9; // v=19

	return v;
}
