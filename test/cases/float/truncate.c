// RUN: %ucc -S -o- %s | grep 'cvtsd2ss'
// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

void fp_f(float f)
{
}

fp_d(double d)
{
	float f = d;
	fp_f(f);
}

main()
{
	volatile float fp = 1.61803;
	int x = fp;

	if(x != 1)
		abort();

	return 0;
}
