// RUN: %ucc -S -o- %s | grep 'cvtss2sd'
// RUN: %ucc -S -o- %s | grep 'cvtsd2ss'
// RUN: %ucc -c %s
// weakish test..

f_f(float f)
{
	void f_d(double);

	f_d(f);
}

void f_d(double d)
{
	f_f(d);
}

main()
{
	float f;
	double d;

	f = 1;

	d = f; // sstosd
	f = d; // sdtoss
}
