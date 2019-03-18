// RUN: %ucc -S -o- %s | grep 'cvtss2sd'

float f(void);

main()
{
	printf("%f\n", /* float -> double */ f());
}
