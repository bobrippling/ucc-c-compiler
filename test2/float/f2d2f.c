// RUN: %ucc -S -o- %s | grep 'cvtss2sd'
// RUN: %ucc -S -o- %s | grep 'cvtsd2ss'
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
