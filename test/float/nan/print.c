// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'nan'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

f(float f)
{
	printf("%f\n", f);
}

main()
{
	f(__builtin_nanf(""));
}
