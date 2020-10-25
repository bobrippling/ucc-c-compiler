// RUN: %ocheck 1 %s
typedef double f_t;

gt(f_t a, f_t b)
{
	return a > b;
}

main()
{
	float a = 2, b = 1;

	return gt(a, b);
}
