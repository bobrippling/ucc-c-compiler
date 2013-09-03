// RUN: %ucc -S -o- %s | grep 'cvtsd2ss'

void fp_f(float);

fp_d(double d)
{
	float f = d;
	fp_f(f);
}
