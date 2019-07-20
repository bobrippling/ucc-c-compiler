// RUN: %ucc -o %t %s
// RUN: %t | %output_check '1 2.00 3 4.00'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

f(int i, float f, int j, float g);

f(int i, float f, int j, float g)
{
	printf("%d %.2f %d %.2f\n", i, f, j, g);
}

main()
{
	f(1, 2, 3, 4);
}
