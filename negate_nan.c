// RUN: %output_check %s

int printf(const char *, ...);

static double negate(double x)
{
	return -x;
}

int main()
{
	double nan = __builtin_nan("");
	double nnan = negate(nan);

	printf("%f %f\n", nan, nnan);
	//TODO: inf

	// STDOUT: nan -nan
}
