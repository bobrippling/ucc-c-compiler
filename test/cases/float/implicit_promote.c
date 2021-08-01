// RUN: %ucc -S -o- %s | grep 'cvtss2sd'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

float f(void);

main()
{
	printf("%f\n", /* float -> double */ f());
}
