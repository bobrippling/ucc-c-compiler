// RUN: %ucc -o %t %s
// RUN: %t | %output_check 3.20

printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	long double x = 3.2L;

	printf("%.2Lf\n", x);
}
